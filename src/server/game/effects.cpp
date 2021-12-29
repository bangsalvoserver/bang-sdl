#include "common/effects.h"
#include "common/requests.h"

#include "player.h"
#include "game.h"

namespace banggame {

    using namespace enums::flag_operators;

    void effect_bang::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->queue_request<request_type::bang>(origin_card, origin, target, flags);
    }

    void effect_bangcard::verify(card *origin_card, player *origin, player *target) const {
        if (origin->m_game->has_scenario(scenario_flags::sermon)) {
            throw game_error("ERROR_SCENARIO_AT_PLAY", origin->m_game->m_scenario_cards.back());
        }
    }

    void effect_bangcard::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->queue_event<event_type::on_play_bang>(origin);
        target->m_game->queue_event<event_type::delayed_action>([=, flags = this->flags]{
            request_bang req{origin_card, origin, target, flags};
            req.is_bang_card = true;
            origin->apply_bang_mods(req);
            origin->m_game->queue_request(std::move(req));
        });
    }

    void effect_aim::on_play(card *origin_card, player *origin) {
        origin->add_bang_mod([](request_bang &req) {
            ++req.bang_damage;
        });
    }

    bool effect_missed::can_respond(card *origin_card, player *origin) const {
        if (origin->m_game->top_request_is(request_type::bang, origin)) {
            auto &req = origin->m_game->top_request().get<request_type::bang>();
            return !req.unavoidable;
        }
        return origin->m_game->top_request_is(request_type::ricochet, origin);
    }

    void effect_missed::on_play(card *origin_card, player *origin) {
        if (origin->m_game->top_request_is(request_type::bang, origin)) {
            auto &req = origin->m_game->top_request().get<request_type::bang>();
            if (0 == --req.bang_strength) {
                origin->m_game->instant_event<event_type::on_missed>(req.origin_card, req.origin, req.target, req.is_bang_card);
                origin->m_game->pop_request();
            }
        } else {
            origin->m_game->pop_request();
        }
    }

    void effect_missedcard::verify(card *origin_card, player *origin) const {
        if (origin->m_cant_play_missedcard) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    static auto barrels_used(request_holder &holder) {
        return enums::visit([](auto &req) -> std::vector<card *> * {
            if constexpr (requires { req.barrels_used; }) {
                return &req.barrels_used;
            }
            return nullptr;
        }, holder);
    };

    bool effect_barrel::can_respond(card *origin_card, player *origin) const {
        if (effect_missed().can_respond(origin_card, origin)) {
            auto *vec = barrels_used(origin->m_game->top_request());
            return std::ranges::find(*vec, origin_card) == vec->end();
        }
        return false;
    }

    void effect_barrel::on_play(card *origin_card, player *target) {
        barrels_used(target->m_game->top_request())->push_back(origin_card);
        target->m_game->draw_check_then(target, origin_card, [=](card *drawn_card) {
            if (target->get_card_suit(drawn_card) == card_suit_type::hearts) {
                effect_missed().on_play(origin_card, target);
            }
        });
    }

    void effect_banglimit::verify(card *origin_card, player *origin) const {
        if (!origin->m_infinite_bangs && origin->m_bangs_played >= origin->m_bangs_per_turn) {
            throw game_error("ERROR_ONE_BANG_PER_TURN");
        }
    }

    void effect_banglimit::on_play(card *origin_card, player *origin) {
        ++origin->m_bangs_played;
    }

    void effect_indians::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->queue_request<request_type::indians>(origin_card, origin, target, flags);
    }

    void effect_duel::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->queue_request<request_type::duel>(origin_card, origin, target, origin, flags);
    }

    bool effect_bangresponse::can_respond(card *origin_card, player *origin) const {
        if (origin->m_game->has_scenario(scenario_flags::sermon)
            && origin == origin->m_game->m_playing) return false;
        return origin->m_game->top_request_is(request_type::duel, origin)
            || origin->m_game->top_request_is(request_type::indians, origin);
    }

    void effect_bangresponse::on_play(card *origin_card, player *target) {
        switch (target->m_game->top_request().enum_index()) {
        case request_type::duel: {
            auto &req = target->m_game->top_request().get<request_type::duel>();
            card *origin_card = req.origin_card;
            player *origin = req.origin;
            player *respond_to = req.respond_to;
            player *target = req.target;
            target->m_game->pop_request_noupdate();
            target->m_game->queue_request<request_type::duel>(origin_card, origin, respond_to, target);
            break;
        }
        case request_type::indians:
            target->m_game->pop_request();
        }
    }

    bool effect_bangresponse_onturn::can_respond(card *origin_card, player *origin) const {
        return effect_bangresponse::can_respond(origin_card, origin)
            && origin == origin->m_game->m_playing;
    }

    bool effect_bangmissed::can_respond(card *origin_card, player *origin) const {
        return effect_missed().can_respond(origin_card, origin)
            || effect_bangresponse().can_respond(origin_card, origin);
    }

    void effect_bangmissed::on_play(card *origin_card, player *target) {
        switch (target->m_game->top_request().enum_index()) {
        case request_type::bang:
            effect_missed().on_play(origin_card, target);
            break;
        case request_type::duel:
        case request_type::indians:
            effect_bangresponse().on_play(origin_card, target);
            break;
        }
    }

    void effect_generalstore::on_play(card *origin_card, player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            origin->m_game->draw_card_to(card_pile_type::selection);
        }
        origin->m_game->queue_request<request_type::generalstore>(origin_card, origin, origin);
    }

    void effect_heal::on_play(card *origin_card, player *origin, player *target) {
        target->heal(1);
    }

    void effect_damage::verify(card *origin_card, player *origin, player *target) const {
        if (target->m_hp <= 1) {
            throw game_error("ERROR_CANT_SELF_DAMAGE");
        }
    }

    void effect_damage::on_play(card *origin_card, player *origin, player *target) {
        target->damage(origin_card, origin, 1);
    }

    void effect_beer::verify(card *origin_card, player *origin, player *target) const {
        if (origin->m_game->has_scenario(scenario_flags::reverend)) {
            throw game_error("ERROR_SCENARIO_AT_PLAY", origin->m_game->m_scenario_cards.back());
        }
    }

    void effect_beer::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->queue_event<event_type::on_play_beer>(target);
        if (target->m_game->m_players.size() <= 2 || target->m_game->num_alive() > 2) {
            target->heal(1);
            target->m_game->instant_event<event_type::on_play_beer_heal>(target);
        }
    }

    bool effect_deathsave::can_respond(card *origin_card, player *origin) const {
        if (origin->m_game->top_request_is(request_type::death, origin)) {
            auto &req = origin->m_game->top_request().get<request_type::death>();
            return req.draw_attempts.empty();
        }
        return false;
    }

    void effect_deathsave::on_play(card *origin_card, player *origin) {
        if (origin->m_hp > 0) {
            origin->m_game->pop_request();
        }
    }

    void effect_destroy::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        if (origin != target && target->can_escape(origin, origin_card, flags)) {
            target->m_game->queue_request<request_type::destroy>(origin_card, origin, target, target_card, flags);
        } else {
            target->m_game->instant_event<event_type::on_discard_card>(origin, target, target_card);
            auto effect_end_pos = std::ranges::find(target->m_game->m_pending_events, enums::indexof(event_type::on_effect_end), &event_args::index);
            if (effect_end_pos == target->m_game->m_pending_events.end()) {
                target->discard_card(target_card);
            } else {
                target->m_game->m_pending_events.emplace(effect_end_pos, std::in_place_index<enums::indexof(event_type::delayed_action)>,
                [=]{
                    target->discard_card(target_card);
                });
            }
        }
    }

    void effect_steal::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        if (origin != target && target->can_escape(origin, origin_card, flags)) {
            target->m_game->queue_request<request_type::steal>(origin_card, origin, target, target_card, flags);
        } else {
            target->m_game->instant_event<event_type::on_discard_card>(origin, target, target_card);
            auto effect_end_pos = std::ranges::find(target->m_game->m_pending_events, enums::indexof(event_type::on_effect_end), &event_args::index);
            if (effect_end_pos == target->m_game->m_pending_events.end()) {
                origin->steal_card(target, target_card);
            } else {
                target->m_game->m_pending_events.emplace(effect_end_pos, std::in_place_index<enums::indexof(event_type::delayed_action)>,
                [=]{
                    origin->steal_card(target, target_card);
                });
            }
        }
    }

    void effect_virtual_destroy::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->discard_card(target_card);
        target->m_virtual.emplace(target_card, *target_card);
    }

    void effect_virtual_copy::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->m_virtual.emplace(target_card, *target_card);
        target->m_virtual->virtual_card.suit = card_suit_type::none;
        target->m_virtual->virtual_card.value = card_value_type::none;
    }

    void effect_virtual_clear::on_play(card *origin_card, player *origin) {
        origin->m_game->queue_event<event_type::delayed_action>([=]{
            origin->m_virtual.reset();
        });
    }

    void effect_startofturn::verify(card *origin_card, player *origin) const {
        if (!origin->m_start_of_turn) {
            throw game_error("ERROR_NOT_START_OF_TURN");
        }
    }

    void effect_draw::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->draw_card_to(card_pile_type::player_hand, target);
    }

    struct draw_atend_handler {
        player *origin;
        int ncards = 1;

        void operator()(player *target, card *origin_card) {
            if (origin == target) {
                for (int i=0; i<ncards; ++i) {
                    target->m_game->draw_card_to(card_pile_type::player_hand, target);
                }
                target->m_game->remove_events(origin_card);
            }
        }
    };

    void effect_draw_atend::on_play(card *origin_card, player *origin, player *target) {
        auto it = origin->m_game->m_event_handlers.find(origin_card);
        if (it == origin->m_game->m_event_handlers.end()) {
            origin->m_game->add_event<event_type::on_play_card_end>(origin_card, draw_atend_handler{target});
        } else {
            ++it->second.get<event_type::on_play_card_end>().target<draw_atend_handler>()->ncards;
        }
    }

    void effect_draw_discard::verify(card *origin_card, player *origin, player *target) const {
        if (target->m_game->m_discards.empty()) {
            throw game_error("ERROR_DISCARD_PILE_EMPTY");
        }
    }

    void effect_draw_discard::on_play(card *origin_card, player *origin, player *target) {
        target->add_to_hand(target->m_game->m_discards.back());
    }

    void effect_draw_rest::on_play(card *origin_card, player *target) {
        while (target->m_num_drawn_cards < target->m_num_cards_to_draw) {
            ++target->m_num_drawn_cards;
            card *drawn_card = target->m_game->draw_phase_one_card_to(card_pile_type::player_hand, target);
            target->m_game->instant_event<event_type::on_card_drawn>(target, drawn_card);
        }
        target->m_has_drawn = true;
        target->m_game->queue_event<event_type::post_draw_cards>(target);
    }

    void effect_draw_done::on_play(card *origin_card, player *target) {
        target->m_has_drawn = true;
        target->m_game->queue_event<event_type::post_draw_cards>(target);
    }

    void effect_draw_skip::verify(card *origin_card, player *target) const {
        if (target->m_num_drawn_cards >= target->m_num_cards_to_draw) {
            throw game_error("ERROR_PLAYER_MUST_NOT_DRAW");
        }
    }

    void effect_draw_skip::on_play(card *origin_card, player *target) {
        if (++target->m_num_drawn_cards == target->m_num_cards_to_draw) {
            target->m_has_drawn = true;
            target->m_game->queue_event<event_type::post_draw_cards>(target);
        }
    }

    void effect_backfire::verify(card *origin_card, player *origin) const {
        if (origin->m_game->m_requests.empty() || !origin->m_game->top_request().origin()) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_backfire::on_play(card *origin_card, player *origin) {
        origin->m_game->queue_request<request_type::bang>(origin_card, origin, origin->m_game->top_request().origin(), flags | effect_flags::single_target);
    }

    void effect_bandidos::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->queue_request<request_type::bandidos>(origin_card, origin, target, flags);
    }

    void effect_tornado::on_play(card *origin_card, player *origin, player *target) {
        if (target->num_hand_cards() == 0) {
            target->m_game->queue_event<event_type::delayed_action>([=]{
                target->m_game->draw_card_to(card_pile_type::player_hand, target);
                target->m_game->draw_card_to(card_pile_type::player_hand, target);
            });
        } else {
            target->m_game->queue_request<request_type::tornado>(origin_card, origin, target, flags);
        }
    }

    void effect_poker::on_play(card *origin_card, player *origin) {
        auto target = origin;
        flags |= effect_flags::single_target & static_cast<effect_flags>(-(std::ranges::count_if(origin->m_game->m_players, [&](const player &p) {
            return &p != origin && p.alive() && !p.m_hand.empty();
        }) == 1));
        while(true) {
            target = origin->m_game->get_next_player(target);
            if (target == origin) break;
            if (!target->m_hand.empty()) {
                origin->m_game->queue_request<request_type::poker>(origin_card, origin, target, flags);
            }
        };
        origin->m_game->queue_event<event_type::delayed_action>([=]{
            if (std::ranges::find(origin->m_game->m_selection, card_value_type::value_A, &card::value) != origin->m_game->m_selection.end()) {
                while (!target->m_game->m_selection.empty()) {
                    origin->m_game->move_to(target->m_game->m_selection.front(), card_pile_type::discard_pile);
                }
            } else if (origin->m_game->m_selection.size() <= 2) {
                while (!origin->m_game->m_selection.empty()) {
                    origin->add_to_hand(origin->m_game->m_selection.front());
                }
            } else {
                origin->m_game->queue_request<request_type::poker_draw>(origin_card, origin);
            }
        });
    }

    bool effect_saved::can_respond(card *origin_card, player *origin) const {
        if (origin->m_game->top_request_is(request_type::damaging)) {
            auto &req = origin->m_game->top_request().get<request_type::damaging>();
            return req.origin != origin;
        }
        return false;
    }

    void effect_saved::on_play(card *origin_card, player *origin) {
        auto &req = origin->m_game->top_request().get<request_type::damaging>();
        player *saved = req.origin;
        if (0 == --req.damage) {
            origin->m_game->pop_request();
        }
        origin->m_game->queue_event<event_type::delayed_action>([=]{
            if (saved->alive()) {
                origin->m_game->queue_request<request_type::saved>(origin_card, origin, saved);
            }
        });
    }

    bool effect_escape::can_respond(card *origin_card, player *origin) const {
        return !origin->m_game->m_requests.empty()
            && bool(origin->m_game->top_request().flags() & effect_flags::escapable);
    }

    void effect_escape::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request();
    }

    void effect_rum::on_play(card *origin_card, player *origin) {
        std::vector<card_suit_type> suits;
        for (int i=0; i < 3 + origin->m_num_checks; ++i) {
            suits.push_back(origin->get_card_suit(origin->m_game->draw_card_to(card_pile_type::selection)));
        }
        while (!origin->m_game->m_selection.empty()) {
            card *drawn_card = origin->m_game->m_selection.front();
            origin->m_game->move_to(drawn_card, card_pile_type::discard_pile);
            origin->m_game->queue_event<event_type::on_draw_check>(origin, drawn_card);
        }
        std::sort(suits.begin(), suits.end());
        origin->heal(std::unique(suits.begin(), suits.end()) - suits.begin());
    }

    void effect_goldrush::on_play(card *origin_card, player *origin) {
        origin->m_game->add_event<event_type::on_turn_end>(origin_card, [=](player *p) {
            if (p == origin) {
                origin->heal(origin->m_max_hp);
                origin->m_game->remove_events(origin_card);
            }
        });
        origin->pass_turn(origin);
    }

    static void swap_shop_choice_in(card *origin_card, player *origin, effect_type type) {
        while (!origin->m_game->m_shop_selection.empty()) {
            origin->m_game->move_to(origin->m_game->m_shop_selection.front(), card_pile_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
        }

        auto &vec = origin->m_game->m_hidden_deck;
        for (auto it = vec.begin(); it != vec.end(); ) {
            auto *card = *it;
            if (!card->responses.empty() && card->responses.front().is(type)) {
                it = origin->m_game->move_to(card, card_pile_type::shop_selection, true, nullptr, show_card_flags::no_animation);
            } else {
                ++it;
            }
        }

        origin->m_game->queue_request<request_type::shopchoice>(origin_card, origin);
    }

    void effect_bottle::on_play(card *origin_card, player *origin) {
        swap_shop_choice_in(origin_card, origin, effect_type::bottlechoice);
    }

    void effect_pardner::on_play(card *origin_card, player *origin) {
        swap_shop_choice_in(origin_card, origin, effect_type::pardnerchoice);
    }

    bool effect_shopchoice::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is(request_type::shopchoice, origin);
    }

    void effect_shopchoice::on_play(card *origin_card, player *origin) {
        int n_choice = origin->m_game->m_shop_selection.size();
        while (!origin->m_game->m_shop_selection.empty()) {
            origin->m_game->move_to(origin->m_game->m_shop_selection.front(), card_pile_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
        }

        auto it = origin->m_game->m_hidden_deck.end() - n_choice - 2;
        for (int i=0; i<2; ++i) {
            it = origin->m_game->move_to(*it, card_pile_type::shop_selection, true, nullptr, show_card_flags::no_animation);
        }
        origin->m_game->pop_request();
        origin->m_game->queue_event<event_type::delayed_action>([m_game = origin->m_game]{
            while (m_game->m_shop_selection.size() < 3) {
                m_game->draw_shop_card();
            }
        });
    }

    void effect_pay_cube::verify(card *origin_card, player *origin, player *target, card *target_card) const {
        if (target_card->cubes.size() < std::max<short>(1, args)) {
            throw game_error("ERROR_NOT_ENOUGH_CUBES_ON", origin_card);
        }
    }

    void effect_pay_cube::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->pay_cubes(target_card, std::max<short>(1, args));
    }

    void effect_add_cube::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->add_cubes(target_card, std::max<short>(1, args));
    }

    void effect_reload::on_play(card *origin_card, player *origin) {
        if (origin->can_receive_cubes()) {
            origin->m_game->queue_request<request_type::add_cube>(origin_card, origin, 3);
        }
    }
    
    void effect_rust::on_play(card *origin_card, player *origin, player *target) {
        if (target->can_escape(origin, origin_card, flags)) {
            origin->m_game->queue_request<request_type::rust>(origin_card, origin, target, flags);
        } else {
            target->m_game->queue_event<event_type::delayed_action>([=]{
                target->move_cubes(target->m_characters.front(), origin->m_characters.front(), 1);
                for (auto &c : target->m_table | std::views::reverse) {
                    if (c->color == card_color_type::orange) {
                        target->move_cubes(c, origin->m_characters.front(), 1);
                    }
                }
            });
        }
    }

    void effect_bandolier::verify(card *origin_card, player *origin) const {
        if (origin->m_bangs_played == 0) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_bandolier::on_play(card *origin_card, player *origin) {
        ++origin->m_bangs_per_turn;
    }

    void effect_belltower::verify(card *origin_card, player *origin) const {
        if (origin->m_belltower) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_belltower::on_play(card *origin_card, player *origin) {
        ++origin->m_belltower;
    }

    void effect_doublebarrel::on_play(card *origin_card, player *origin) {
        origin->add_bang_mod([=](request_bang &req) {
            if (origin->get_card_suit(req.origin_card) == card_suit_type::diamonds) {
                req.unavoidable = true;
            }
        });
    }

    void effect_thunderer::on_play(card *origin_card, player *origin) {
        origin->add_bang_mod([=](request_bang &req) {
            req.cleanup_function = [=]{
                card *bang_card = origin->m_virtual ? origin->m_virtual->corresponding_card : req.origin_card;
                origin->add_to_hand(bang_card);
            };
        });
    }

    void effect_buntlinespecial::on_play(card *origin_card, player *p) {
        p->add_bang_mod([=](request_bang &req) {
            if (std::ranges::any_of(p->m_characters, [](const character *c) {
                return std::ranges::find(c->equips, equip_type::slab_the_killer, &equip_holder::type) != c->equips.end();
            })) {
                ++req.bang_strength;
            }
            p->m_game->add_event<event_type::on_missed>(origin_card, [=](card *bang_card, player *origin, player *target, bool is_bang) {
                if (target && origin == p && is_bang && !target->m_hand.empty()) {
                    target->m_game->queue_request<request_type::discard>(origin_card, origin, target);
                }
            });
            req.cleanup_function = [=]{
                p->m_game->remove_events(origin_card);
            };
        });
    }

    void effect_bigfifty::on_play(card *origin_card, player *p) {
        p->m_game->disable_table_cards();
        p->m_game->disable_characters();
        p->add_bang_mod([p](request_bang &req) {
            req.cleanup_function = [p]{
                p->m_game->enable_table_cards();
                p->m_game->enable_characters();
            };
        });
    }

    void effect_flintlock::on_play(card *origin_card, player *p) {
        p->m_game->add_event<event_type::on_missed>(origin_card, [=](card *origin_card, player *origin, player *target, bool is_bang) {
            if (origin == p) {
                origin->add_to_hand(origin_card);
            }
        });
        p->m_game->top_request().get<request_type::bang>().cleanup_function = [=]{
            p->m_game->remove_events(origin_card);
        };
    }

    void effect_duck::on_play(card *origin_card, player *origin) {
        origin->add_to_hand(origin_card);
    }

    struct squaw_handler {
        card *origin_card;
        player *origin;
        player *target;
        card *target_card;
        effect_flags flags;
        bool steal;

        void operator()(player *p, card *c) {
            if (p == origin && c == origin_card) {
                if (steal) {
                    effect_steal e;
                    e.flags = flags;
                    e.on_play(origin_card, origin, target, target_card);
                } else {
                    effect_destroy e;
                    e.flags = flags;
                    e.on_play(origin_card, origin, target, target_card);
                }
                origin->m_game->remove_events(origin_card);
            }
        }
    };

    void effect_squaw_destroy::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        origin->m_game->add_event<event_type::on_play_card_end>(origin_card,
            squaw_handler{origin_card, origin, target, target_card, flags, false});
    }

    void effect_squaw_steal::on_play(card *origin_card, player *origin) {
        origin->m_game->m_event_handlers.find(origin_card)->second
            .get<event_type::on_play_card_end>().target<squaw_handler>()->steal = true;
    }

    void effect_tumbleweed::on_equip(player *origin, card *target_card) {
        origin->m_game->add_event<event_type::trigger_tumbleweed>(target_card, [=](card *origin_card, card *drawn_card) {
            origin->m_game->add_request<request_type::tumbleweed>(target_card, origin, drawn_card, origin_card);
            origin->m_game->m_current_check->no_auto_resolve = true;
        });
    }

    bool effect_tumbleweed::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is(request_type::tumbleweed);
    }

    void effect_tumbleweed::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request();
        origin->m_game->do_draw_check();
    }

    void timer_tumbleweed::on_finished() {
        target->m_game->pop_request();
        target->m_game->m_current_check->function(drawn_card);
        target->m_game->m_current_check.reset();
    }

    void effect_sniper::on_play(card *origin_card, player *origin, player *target) {
        request_bang req{origin_card, origin, target, flags};
        req.bang_strength = 2;
        target->m_game->queue_request(std::move(req));
    }

    void effect_ricochet::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->m_game->queue_request<request_type::ricochet>(origin_card, origin, target, target_card, flags);
    }
}