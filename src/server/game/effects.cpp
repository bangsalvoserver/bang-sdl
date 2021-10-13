#include "common/effects.h"
#include "common/requests.h"

#include "player.h"
#include "game.h"

namespace banggame {

    void effect_bang::on_play(player *origin, player *target) {
        target->m_game->queue_request<request_type::bang>(origin, target);
    }

    void effect_bangcard::on_play(player *origin, player *target) {
        auto &req = target->m_game->queue_request<request_type::bang>(origin, target);
        req.is_bang_card = true;
        req.bang_damage = origin->m_bang_damage;
        target->m_game->instant_event<event_type::apply_bang_modifiers>(origin, req);
    }

    void effect_aim::on_play(player *origin) {
        ++origin->m_bang_damage;
    }

    bool effect_missed::can_respond(player *origin) const {
        if (origin->m_game->top_request_is(request_type::bang, origin)) {
            auto &req = origin->m_game->top_request().get<request_type::bang>();
            return !req.unavoidable;
        }
        return false;
    }

    void effect_missed::on_play(player *origin) {
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

    void effect_barrel::on_play(player *origin, player *target, int card_id) {
        auto &req = target->m_game->top_request().get<request_type::bang>();
        if (std::ranges::find(req.barrels_used, card_id) == std::ranges::end(req.barrels_used)) {
            req.barrels_used.push_back(card_id);
            target->m_game->draw_check_then(target, [target](card_suit_type suit, card_value_type) {
                if (suit == card_suit_type::hearts) {
                    effect_missed().on_play(target);
                }
            });
        }
    }

    bool effect_banglimit::can_play(player *target) const {
        return target->can_play_bang();
    }

    void effect_banglimit::on_play(player *origin) {
        ++origin->m_bangs_played;
    }

    void effect_indians::on_play(player *origin, player *target) {
        target->m_game->queue_request<request_type::indians>(origin, target);
    }

    void effect_duel::on_play(player *origin, player *target) {
        target->m_game->queue_request<request_type::duel>(origin, target);
    }

    bool effect_bangresponse::can_respond(player *origin) const {
        return origin->m_game->top_request_is(request_type::duel, origin)
            || origin->m_game->top_request_is(request_type::indians, origin);
    }

    void effect_bangresponse::on_play(player *target) {
        switch (target->m_game->top_request().enum_index()) {
        case request_type::duel: {
            player *origin = target->m_game->top_request().origin();
            target->m_game->pop_request_noupdate();
            target->m_game->queue_request<request_type::duel>(target, origin);
            break;
        }
        case request_type::indians:
            target->m_game->pop_request();
        }
    }

    bool effect_bangmissed::can_respond(player *origin) const {
        return effect_missed().can_respond(origin) || effect_bangresponse().can_respond(origin);
    }

    void effect_bangmissed::on_play(player *target) {
        switch (target->m_game->top_request().enum_index()) {
        case request_type::bang:
            effect_missed().on_play(target);
            break;
        case request_type::duel:
        case request_type::indians:
            effect_bangresponse().on_play(target);
            break;
        }
    }

    void effect_generalstore::on_play(player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            origin->m_game->add_to_temps(origin->m_game->draw_card());
        }
        origin->m_game->queue_request<request_type::generalstore>(origin, origin);
    }

    void effect_heal::on_play(player *origin, player *target) {
        target->heal(1);
    }

    bool effect_damage::can_play(player *target) const {
        return target->m_hp > 1;
    }

    void effect_damage::on_play(player *origin, player *target) {
        target->damage(origin, 1);
    }

    void effect_beer::on_play(player *origin, player *target) {
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

    void effect_deathsave::on_play(player *origin) {
        if (origin->m_hp > 0) {
            origin->m_game->pop_request();
        }
    }

    void effect_destroy::on_play(player *origin, player *target, int card_id) {
        target->m_game->queue_event<event_type::on_discard_card>(origin, target, card_id);
        target->m_game->queue_event<event_type::delayed_action>([=]{
            target->discard_card(card_id);
        });
    }

    void effect_virtual_destroy::on_play(player *origin, player *target, int card_id) {
        target->m_virtual = std::make_pair(card_id, target->discard_card(card_id));
    }

    void effect_virtual_copy::on_play(player *origin, player *target, int card_id) {
        auto copy = target->find_card(card_id);
        copy.suit = card_suit_type::none;
        copy.value = card_value_type::none;
        target->m_virtual = std::make_pair(card_id, std::move(copy));
    }

    void effect_virtual_clear::on_play(player *origin) {
        origin->m_virtual.reset();
    }

    void effect_steal::on_play(player *origin, player *target, int card_id) {
        target->m_game->queue_event<event_type::on_discard_card>(origin, target, card_id);
        target->m_game->queue_event<event_type::delayed_action>([=]{
            origin->steal_card(target, card_id);
        });
    }

    void effect_draw::on_play(player *origin, player *target) {
        target->add_to_hand(target->m_game->draw_card());
    }

    bool effect_draw_discard::can_play(player *target) const {
        return ! target->m_game->m_discards.empty();
    }

    void effect_draw_discard::on_play(player *origin, player *target) {
        target->add_to_hand(target->m_game->draw_from_discards());
    }

    void effect_draw_rest::on_play(player *target) {
        for (; target->m_num_drawn_cards<target->m_num_cards_to_draw; ++target->m_num_drawn_cards) {
            target->add_to_hand(target->m_game->draw_card());
        }
    }

    void effect_draw_done::on_play(player *target) {
        target->m_num_drawn_cards = target->m_num_cards_to_draw;
    }

    bool effect_draw_skip::can_play(player *target) const {
        return target->m_num_drawn_cards < target->m_num_cards_to_draw;
    }

    void effect_draw_skip::on_play(player *target) {
        ++target->m_num_drawn_cards;
    }

    void effect_bandidos::on_play(player *origin, player *target) {
        target->m_game->queue_request<request_type::bandidos>(origin, target);
    }

    void effect_tornado::on_play(player *origin, player *target) {
        if (target->num_hand_cards() == 0) {
            target->m_game->queue_event<event_type::delayed_action>([=]{
                target->add_to_hand(target->m_game->draw_card());
                target->add_to_hand(target->m_game->draw_card());
            });
        } else {
            target->m_game->queue_request<request_type::tornado>(origin, target);
        }
    }

    void effect_poker::on_play(player *origin) {
        auto next = origin;
        do {
            next = origin->m_game->get_next_player(next);
            if (next == origin) return;
        } while (next->m_hand.empty());
        origin->m_game->queue_request<request_type::poker>(origin, next);
    }

    bool effect_saved::can_respond(player *origin) const {
        if (origin->m_game->top_request_is(request_type::damaging)) {
            auto &t = origin->m_game->top_request().get<request_type::damaging>();
            return t.target != origin;
        }
        return false;
    }

    void effect_saved::on_play(player *origin) {
        auto &timer = origin->m_game->top_request().get<request_type::damaging>();
        auto fun = [origin, target = timer.target]{
            if (target->alive()) {
                origin->m_game->queue_request<request_type::saved>(nullptr, origin).saved = target;
            }
        };
        if (0 == --timer.damage) {
            origin->m_game->pop_request();
        }
        origin->m_game->queue_event<event_type::delayed_action>(std::move(fun));
    }

    bool effect_flight::can_respond(player *origin) const {
        return origin->m_game->top_request_is(request_type::flightable, origin);
    }

    void effect_flight::on_play(player *origin) {
        origin->m_game->pop_request();
    }
}