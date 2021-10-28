#include "common/effects.h"
#include "common/requests.h"

#include "player.h"
#include "game.h"

namespace banggame {

    using namespace enums::flag_operators;

    void effect_bang::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->queue_request<request_type::bang>(origin_card_id, origin, target, escapable);
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
        target->m_game->queue_request<request_type::indians>(origin_card_id, origin, target, escapable);
    }

    void effect_duel::on_play(int origin_card_id, player *origin, player *target) {
        target->m_game->queue_request<request_type::duel>(origin_card_id, origin, target, escapable);
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
        if (escapable && origin != target && origin->m_game->has_expansion(card_expansion_type::valleyofshadows)) {
            auto &req = target->m_game->queue_request<request_type::destroy>(origin_card_id, origin, target, true);
            req.card_id = card_id;
        } else {
            target->m_game->queue_event<event_type::on_discard_card>(origin, target, card_id);
            target->m_game->queue_event<event_type::delayed_action>([=]{
                target->discard_card(card_id);
            });
        }
    }

    void effect_steal::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        if (escapable && origin != target && origin->m_game->has_expansion(card_expansion_type::valleyofshadows)) {
            auto &req = target->m_game->queue_request<request_type::steal>(origin_card_id, origin, target, true);
            req.card_id = card_id;
        } else {
            target->m_game->queue_event<event_type::on_discard_card>(origin, target, card_id);
            target->m_game->queue_event<event_type::delayed_action>([=]{
                origin->steal_card(target, card_id);
            });
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
        target->m_game->queue_request<request_type::bandidos>(origin_card_id, origin, target, escapable);
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
        auto next = origin;
        do {
            next = origin->m_game->get_next_player(next);
            if (next == origin) return;
        } while (next->m_hand.empty());
        origin->m_game->queue_request<request_type::poker>(origin_card_id, origin, next, escapable);
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
        return !origin->m_game->m_requests.empty() && origin->m_game->top_request().escapable();
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
    }

    bool effect_pay_cube::can_play(int origin_card_id, player *origin, player *target, int card_id) const {
        if (card_id == target->m_characters.front().id) {
            auto &card = target->m_characters.front();
            return card.cubes.size() >= args;
        } else {
            auto &card = target->find_card(card_id);
            return card.cubes.size() >= args;
        }
    }

    void effect_pay_cube::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        if (card_id == target->m_characters.front().id) {
            auto &card = target->m_characters.front();
            target->m_game->pay_cubes(card, args);
        } else {
            auto &card = target->find_card(card_id);
            target->m_game->pay_cubes(card, args);
            if (card.cubes.empty()) {
                target->m_game->queue_event<event_type::delayed_action>([=]{
                    target->discard_card(card_id);
                });
            }
        }
    }

    void effect_add_cube::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        if (card_id == target->m_characters.front().id) {
            auto &card = target->m_characters.front();
            target->m_game->add_cubes(card, args);
        } else {
            auto &card = target->find_card(card_id);
            target->m_game->add_cubes(card, args);
        }
    }

    void effect_reload::on_play(int origin_card_id, player *origin) {
        if (origin->can_receive_cubes()) {
            origin->m_game->queue_request<request_type::add_cube>(origin_card_id, nullptr, origin).ncubes = 3;
        }
    }

    bool effect_bandolier::can_play(int origin_card_id, player *origin) const {
        return origin->m_bangs_played > 0 && origin->m_bangs_per_turn == 1;
    }

    void effect_bandolier::on_play(int origin_card_id, player *origin) {
        origin->m_bangs_per_turn = 2;
    }

    void effect_doublebarrel::on_play(int origin_card_id, player *origin) {
        origin->add_bang_mod([=](request_bang &req) {
            if (auto it = std::ranges::find(origin->m_game->m_discards | std::views::reverse, req.origin_card_id, &deck_card::id);
                it != origin->m_game->m_discards.rend() && it->suit == card_suit_type::diamonds) {
                req.unavoidable = true;
            }
        });
    }


}