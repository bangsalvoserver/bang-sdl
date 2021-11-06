#include "common/effects.h"
#include "common/requests.h"

#include "player.h"
#include "game.h"

namespace banggame {

    using namespace enums::flag_operators;

    void effect_bang::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->queue_request<request_type::bang>(origin_card_id, origin, target, flags);
    }

    void effect_bangcard::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->queue_event<event_type::on_play_bang>(origin);
        target->m_game->queue_event<event_type::delayed_action>([=]{
            request_bang req{origin_card_id, origin, target};
            req.is_bang_card = true;
            origin->apply_bang_mods(req);
            origin->m_game->queue_request(std::move(req));
        });
    }

    void effect_aim::on_play(int origin_card_id, player *origin) {
        origin->add_bang_mod([](request_bang &req) {
            ++req.bang_damage;
        });
    }

    bool effect_missed::can_respond(player *origin) const {
        if (origin->m_game->top_request_is(request_type::bang, origin)) {
            auto &req = origin->m_game->top_request().get<request_type::bang>();
            return !req.unavoidable;
        }
        return false;
    }

    void effect_missed::on_play(int origin_card_id, player *origin) {
        auto &req = origin->m_game->top_request().get<request_type::bang>();
        if (0 == --req.bang_strength) {
            origin->m_game->instant_event<event_type::on_missed>(req.origin, req.target, req.is_bang_card);
            origin->m_game->pop_request();
        }
    }

    bool effect_missedcard::can_respond(player *origin) const {
        return !origin->m_cant_play_missedcard && effect_missed().can_respond(origin);
    }

    bool effect_barrel::can_respond(player *origin) const {
        return effect_missed().can_respond(origin);
    }

    void effect_barrel::on_play(int card_id, player *target) {
        auto &req = target->m_game->top_request().get<request_type::bang>();
        if (std::ranges::find(req.barrels_used, card_id) == std::ranges::end(req.barrels_used)) {
            req.barrels_used.push_back(card_id);
            target->m_game->draw_check_then(target, [=](card_suit_type suit, card_value_type) {
                if (suit == card_suit_type::hearts) {
                    effect_missed().on_play(card_id, target);
                }
            });
        }
    }

    bool effect_banglimit::can_play(int origin_card_id, player *origin) const {
        return origin->can_play_bang();
    }

    void effect_banglimit::on_play(int origin_card_id, player *origin) {
        ++origin->m_bangs_played;
    }

    void effect_indians::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->queue_request<request_type::indians>(origin_card_id, origin, target, flags);
    }

    void effect_duel::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->queue_request<request_type::duel>(origin_card_id, origin, target, flags);
    }

    bool effect_bangresponse::can_respond(player *origin) const {
        return origin->m_game->top_request_is(request_type::duel, origin)
            || origin->m_game->top_request_is(request_type::indians, origin);
    }

    void effect_bangresponse::on_play(int origin_card_id, player *target) {
        switch (target->m_game->top_request().enum_index()) {
        case request_type::duel: {
            int origin_card_id = target->m_game->top_request().origin_card_id();
            player *origin = target->m_game->top_request().origin();
            target->m_game->pop_request_noupdate();
            target->m_game->queue_request<request_type::duel>(origin_card_id, target, origin);
            break;
        }
        case request_type::indians:
            target->m_game->pop_request();
        }
    }

    bool effect_bangresponse_onturn::can_respond(player *origin) const {
        return effect_bangresponse::can_respond(origin) && origin == origin->m_game->m_playing;
    }

    bool effect_bangmissed::can_respond(player *origin) const {
        return effect_missed().can_respond(origin) || effect_bangresponse().can_respond(origin);
    }

    void effect_bangmissed::on_play(int origin_card_id, player *target) {
        switch (target->m_game->top_request().enum_index()) {
        case request_type::bang:
            effect_missed().on_play(origin_card_id, target);
            break;
        case request_type::duel:
        case request_type::indians:
            effect_bangresponse().on_play(origin_card_id, target);
            break;
        }
    }

    void effect_generalstore::on_play(int origin_card_id, player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            origin->m_game->draw_card_to(card_pile_type::selection);
        }
        origin->m_game->queue_request<request_type::generalstore>(origin_card_id, origin, origin);
    }

    void effect_heal::on_play(int origin_card_id, player *origin, player *target) {
        target->heal(1);
    }

    bool effect_damage::can_play(int origin_card_id, player *origin, player *target) const {
        return target->m_hp > 1;
    }

    void effect_damage::on_play(int origin_card_id, player *origin, player *target) {
        target->damage(origin_card_id, origin, 1);
    }

    void effect_beer::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->queue_event<event_type::on_play_beer>(target);
        if (target->m_game->m_players.size() <= 2 || target->m_game->num_alive() > 2) {
            target->heal(target->m_beer_strength);
        }
    }

    bool effect_deathsave::can_respond(player *origin) const {
        if (origin->m_game->top_request_is(request_type::death, origin)) {
            auto &req = origin->m_game->top_request().get<request_type::death>();
            return req.draw_attempts.empty();
        }
        return false;
    }

    void effect_deathsave::on_play(int origin_card_id, player *origin) {
        if (origin->m_hp > 0) {
            origin->m_game->pop_request();
        }
    }

    void effect_destroy::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        if (origin != target && target->can_escape(origin_card_id, flags)) {
            target->m_game->queue_request(request_destroy{origin_card_id, origin, target, flags, card_id});
        } else {
            target->m_game->instant_event<event_type::on_discard_card>(origin, target, card_id);
            auto effect_end_pos = std::ranges::find(target->m_game->m_pending_events, enums::indexof(event_type::on_effect_end), &event_args::index);
            if (effect_end_pos == target->m_game->m_pending_events.end()) {
                target->discard_card(card_id);
            } else {
                target->m_game->m_pending_events.emplace(effect_end_pos, std::in_place_index<enums::indexof(event_type::delayed_action)>,
                [=]{
                    target->discard_card(card_id);
                });
            }
        }
    }

    void effect_steal::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        if (origin != target && target->can_escape(origin_card_id, flags)) {
            target->m_game->queue_request(request_steal{origin_card_id, origin, target, flags, card_id});
        } else {
            target->m_game->instant_event<event_type::on_discard_card>(origin, target, card_id);
            auto effect_end_pos = std::ranges::find(target->m_game->m_pending_events, enums::indexof(event_type::on_effect_end), &event_args::index);
            if (effect_end_pos == target->m_game->m_pending_events.end()) {
                origin->steal_card(target, card_id);
            } else {
                target->m_game->m_pending_events.emplace(effect_end_pos, std::in_place_index<enums::indexof(event_type::delayed_action)>,
                [=]{
                    origin->steal_card(target, card_id);
                });
            }
        }
    }

    void effect_virtual_destroy::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        target->m_virtual = std::make_pair(card_id, target->discard_card(card_id));
    }

    void effect_virtual_copy::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        auto copy = target->find_card(card_id);
        copy.suit = card_suit_type::none;
        copy.value = card_value_type::none;
        target->m_virtual = std::make_pair(card_id, std::move(copy));
    }

    void effect_virtual_clear::on_play(int origin_card_id, player *origin) {
        origin->m_virtual.reset();
    }

    void effect_draw::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->draw_card_to(card_pile_type::player_hand, target);
    }

    bool effect_draw_discard::can_play(int origin_card_id, player *origin, player *target) const {
        return ! target->m_game->m_discards.empty();
    }

    void effect_draw_discard::on_play(int origin_card_id, player *origin, player *target) {
        target->add_to_hand(target->m_game->draw_from_discards());
    }

    void effect_draw_rest::on_play(int origin_card_id, player *target) {
        for (; target->m_num_drawn_cards<target->m_num_cards_to_draw; ++target->m_num_drawn_cards) {
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
        }
    }

    void effect_draw_done::on_play(int origin_card_id, player *target) {
        target->m_num_drawn_cards = target->m_num_cards_to_draw;
    }

    bool effect_draw_skip::can_play(int origin_card_id, player *target) const {
        return target->m_num_drawn_cards < target->m_num_cards_to_draw;
    }

    void effect_draw_skip::on_play(int origin_card_id, player *target) {
        ++target->m_num_drawn_cards;
    }

    void effect_bandidos::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->queue_request<request_type::bandidos>(origin_card_id, origin, target, flags);
    }

    void effect_tornado::on_play(int origin_card_id, player *origin, player *target) {
        if (target->num_hand_cards() == 0) {
            target->m_game->queue_event<event_type::delayed_action>([=]{
                target->m_game->draw_card_to(card_pile_type::player_hand, target);
                target->m_game->draw_card_to(card_pile_type::player_hand, target);
            });
        } else {
            target->m_game->queue_request<request_type::tornado>(origin_card_id, origin, target);
        }
    }

    void effect_poker::on_play(int origin_card_id, player *origin) {
        auto target = origin;
        while(true) {
            target = origin->m_game->get_next_player(target);
            if (target == origin) break;
            if (!target->m_hand.empty()) {
                origin->m_game->queue_request<request_type::poker>(origin_card_id, origin, target, flags);
            }
        };
        origin->m_game->queue_event<event_type::delayed_action>([=]{
            if (std::ranges::any_of(origin->m_game->m_selection, [](const deck_card &c) {
                return c.value == card_value_type::value_A;
            })) {
                for (auto &c : target->m_game->m_selection) {
                    origin->m_game->move_to(std::move(c), card_pile_type::discard_pile);
                }
                origin->m_game->m_selection.clear();
            } else if (origin->m_game->m_selection.size() <= 2) {
                for (auto &c : origin->m_game->m_selection) {
                    origin->add_to_hand(std::move(c));
                }
                origin->m_game->m_selection.clear();
            } else {
                origin->m_game->queue_request<request_type::poker_draw>(origin_card_id, origin, origin);
            }
        });
    }

    bool effect_saved::can_respond(player *origin) const {
        if (origin->m_game->top_request_is(request_type::damaging)) {
            auto &t = origin->m_game->top_request().get<request_type::damaging>();
            return t.target != origin;
        }
        return false;
    }

    void effect_saved::on_play(int origin_card_id, player *origin) {
        auto &timer = origin->m_game->top_request().get<request_type::damaging>();
        player *saved = timer.target;
        if (0 == --timer.damage) {
            origin->m_game->pop_request();
        }
        origin->m_game->queue_event<event_type::delayed_action>([=]{
            if (saved->alive()) {
                origin->m_game->queue_request<request_type::saved>(origin_card_id, origin, origin).saved = saved;
            }
        });
    }

    bool effect_escape::can_respond(player *origin) const {
        return !origin->m_game->m_requests.empty()
            && bool(origin->m_game->top_request().flags() & effect_flags::escapable);
    }

    void effect_escape::on_play(int origin_card_id, player *origin) {
        origin->m_game->pop_request();
    }

    struct rum_check_handler {
        player *origin;

        std::array<card_suit_type, 4> checks;
        int count_checks;

        void operator()(card_suit_type suit, card_value_type value) {
            if (count_checks < 3) {
                checks[count_checks++] = suit;
                origin->m_game->draw_check_then(origin, std::move(*this), true);
            } else {
                origin->heal(std::distance(checks.begin(), std::unique(checks.begin(), checks.end())));
            }
        }
    };

    void effect_rum::on_play(int origin_card_id, player *origin) {
        origin->m_game->draw_check_then(origin, rum_check_handler{origin}, true);
    }

    void effect_goldrush::on_play(int origin_card_id, player *origin) {
        origin->m_game->m_next_turn = origin;
        origin->pass_turn();
        origin->m_game->queue_event<event_type::delayed_action>([=] {
            origin->heal(origin->m_max_hp);
        });
    }

    static void swap_shop_choice_in(int origin_card_id, player *origin, effect_type type) {
        for (auto &c : origin->m_game->m_shop_selection) {
            origin->m_game->move_to(std::move(c), card_pile_type::shop_hidden, true, nullptr, show_card_flags::no_animation);
        }
        origin->m_game->m_shop_selection.clear();

        auto &vec = origin->m_game->m_shop_hidden;
        for (auto it = vec.begin(); it != vec.end(); ) {
            if (!it->responses.empty() && it->responses.front().is(type)) {
                origin->m_game->move_to(std::move(*it), card_pile_type::shop_selection, true, nullptr, show_card_flags::no_animation);
                it = vec.erase(it);
            } else {
                ++it;
            }
        }

        origin->m_game->queue_request<request_type::shopchoice>(origin_card_id, nullptr, origin);
    }

    void effect_bottle::on_play(int origin_card_id, player *origin) {
        swap_shop_choice_in(origin_card_id, origin, effect_type::bottlechoice);
    }

    void effect_pardner::on_play(int origin_card_id, player *origin) {
        swap_shop_choice_in(origin_card_id, origin, effect_type::pardnerchoice);
    }

    bool effect_shopchoice::can_respond(player *origin) const {
        return origin->m_game->top_request_is(request_type::shopchoice, origin);
    }

    void effect_shopchoice::on_play(int origin_card_id, player *origin) {
        int n_choice = origin->m_game->m_shop_selection.size();
        for (auto &c : origin->m_game->m_shop_selection) {
            origin->m_game->move_to(std::move(c), card_pile_type::shop_hidden, true, nullptr, show_card_flags::no_animation);
        }
        origin->m_game->m_shop_selection.clear();

        auto end = origin->m_game->m_shop_hidden.end() - n_choice;
        auto begin = end - 2;
        for (auto it = begin; it != end; ++it) {
            origin->m_game->move_to(std::move(*it), card_pile_type::shop_selection, true, nullptr, show_card_flags::no_animation);
        }
        origin->m_game->m_shop_hidden.erase(begin, end);
        origin->m_game->pop_request();
        origin->m_game->queue_event<event_type::delayed_action>([m_game = origin->m_game]{
            while (m_game->m_shop_selection.size() < 3) {
                m_game->draw_shop_card();
            }
        });
    }

    bool effect_pay_cube::can_play(int origin_card_id, player *origin, player *target, int card_id) const {
        card *card = nullptr;
        if (card_id == target->m_characters.front().id) {
            card = &target->m_characters.front();
        } else {
            card = &target->find_card(card_id);
        }
        return card->cubes.size() >= std::max(1, args);
    }

    void effect_pay_cube::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        card *card = nullptr;
        if (card_id == target->m_characters.front().id) {
            card = &target->m_characters.front();
        } else {
            card = &target->find_card(card_id);
        }
        target->pay_cubes(*card, std::max(1, args));
    }

    void effect_add_cube::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        if (card_id == target->m_characters.front().id) {
            auto &card = target->m_characters.front();
            target->add_cubes(card, std::max(1, args));
        } else {
            auto &card = target->find_card(card_id);
            target->add_cubes(card, std::max(1, args));
        }
    }

    void effect_reload::on_play(int origin_card_id, player *origin) {
        if (origin->can_receive_cubes()) {
            origin->m_game->queue_request<request_type::add_cube>(origin_card_id, nullptr, origin).ncubes = 3;
        }
    }
    
    void effect_rust::on_play(int origin_card_id, player *origin, player *target) {
        if (target->can_escape(origin_card_id, flags)) {
            origin->m_game->queue_request<request_type::rust>(origin_card_id, origin, target, flags);
        } else {
            target->m_game->queue_event<event_type::delayed_action>([=]{
                target->move_cubes(target->m_characters.front(), origin->m_characters.front(), 1);
                for (auto &c : target->m_table | std::views::reverse) {
                    if (c.color == card_color_type::orange) {
                        target->move_cubes(c, origin->m_characters.front(), 1);
                    }
                }
            });
        }
    }

    bool effect_bandolier::can_play(int origin_card_id, player *origin) const {
        return origin->m_bangs_played > 0 && origin->m_bangs_per_turn == 1;
    }

    void effect_bandolier::on_play(int origin_card_id, player *origin) {
        origin->m_bangs_per_turn = 2;
    }

    bool effect_belltower::can_play(int origin_card_id, player *origin) const {
        return origin->m_belltower == 0;
    }

    void effect_belltower::on_play(int origin_card_id, player *origin) {
        ++origin->m_belltower;
    }

    void effect_doublebarrel::on_play(int origin_card_id, player *origin) {
        origin->add_bang_mod([=](request_bang &req) {
            auto it = std::ranges::find(origin->m_game->m_discards | std::views::reverse, req.origin_card_id, &deck_card::id);
            if (it->suit == card_suit_type::diamonds) {
                req.unavoidable = true;
            }
        });
    }

    void effect_thunderer::on_play(int origin_card_id, player *origin) {
        origin->add_bang_mod([=](request_bang &req) {
            req.cleanup_function = [=]{
                int bang_id = origin->m_virtual ? origin->m_virtual->first : req.origin_card_id;
                auto it = std::ranges::find(origin->m_game->m_discards | std::views::reverse, bang_id, &deck_card::id);
                origin->add_to_hand(std::move(*it));
                origin->m_game->m_discards.erase(it.base());
            };
        });
    }

    void effect_buntlinespecial::on_play(int origin_card_id, player *p) {
        p->add_bang_mod([=](request_bang &req) {
            p->m_game->add_event<event_type::on_missed>(origin_card_id, [=](player *origin, player *target, bool is_bang) {
                if (target && origin == p && is_bang && !target->m_hand.empty()) {
                    target->m_game->queue_request<request_type::discard>(origin_card_id, origin, target);
                }
            });
            req.cleanup_function = [=]{
                p->m_game->remove_events(origin_card_id);
            };
        });
    }

    void effect_bigfifty::on_play(int origin_card_id, player *p) {
        p->m_game->disable_table_cards(p->id, true);
        p->add_bang_mod([p](request_bang &req) {
            req.cleanup_function = [=]{
                p->m_game->enable_table_cards(p->id);
            };
        });
    }

    void effect_flintlock::on_play(int origin_card_id, player *p) {
        p->m_game->add_event<event_type::on_missed>(origin_card_id, [=](player *origin, player *target, bool is_bang) {
            if (origin == p) {
                auto it = std::ranges::find(origin->m_game->m_discards | std::views::reverse, origin_card_id, &deck_card::id);
                if (it != origin->m_game->m_discards.rend()) {
                    origin->m_game->move_to(std::move(*it), card_pile_type::player_hand, true, origin);
                    origin->m_game->m_discards.erase(it.base());
                }
            }
        });
        p->m_game->top_request().get<request_type::bang>().cleanup_function = [=]{
            p->m_game->remove_events(origin_card_id);
        };
    }

    void effect_duck::on_play(int origin_card_id, player *origin) {
        auto it = std::ranges::find(origin->m_game->m_discards | std::views::reverse, origin_card_id, &deck_card::id);
        origin->m_game->move_to(std::move(*it), card_pile_type::player_hand, true, origin);
        origin->m_game->m_discards.erase(it.base());
    }

    struct squaw_handler {
        int origin_card_id;
        player *origin;
        player *target;
        int card_id;
        effect_flags flags;
        bool steal;

        void operator()(player *p, int id) {
            if (p == origin && id == origin_card_id) {
                origin->m_game->remove_events(origin_card_id);
                if (steal) {
                    effect_steal e;
                    e.flags = flags;
                    e.on_play(origin_card_id, origin, target, card_id);
                } else {
                    effect_destroy e;
                    e.flags = flags;
                    e.on_play(origin_card_id, origin, target, card_id);
                }
            }
        }
    };

    void effect_squaw_destroy::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        origin->m_game->add_event<event_type::on_play_card_end>(origin_card_id,
            squaw_handler{origin_card_id, origin, target, card_id, flags, false});
    }

    void effect_squaw_steal::on_play(int origin_card_id, player *origin) {
        origin->m_game->m_event_handlers.find(origin_card_id)->second
            .get<event_type::on_play_card_end>().target<squaw_handler>()->steal = true;
    }

    void effect_tumbleweed::on_equip(player *origin, int card_id) {
        origin->m_game->add_event<event_type::trigger_tumbleweed>(card_id, [=](card_suit_type suit, card_value_type value) {
            auto &req = origin->m_game->add_request<request_type::tumbleweed>(card_id, origin, origin);
            req.suit = suit;
            req.value = value;
            origin->m_game->m_current_check->no_auto_resolve = true;
        });
    }

    bool effect_tumbleweed::can_respond(player *origin) const {
        return origin->m_game->top_request_is(request_type::tumbleweed);
    }

    void effect_tumbleweed::on_play(int origin_card_id, player *origin) {
        origin->m_game->pop_request();
        origin->m_game->do_draw_check();
    }

    void timer_tumbleweed::on_finished() {
        origin->m_game->pop_request();
        origin->m_game->m_current_check->function(suit, value);
        origin->m_game->m_current_check.reset();
    }

}