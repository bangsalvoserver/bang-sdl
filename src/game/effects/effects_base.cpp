#include "effects_base.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_play_card_action::on_play(card *origin_card, player *origin) {
        origin->play_card_action(origin_card, effect_value == 1);
    }

    void effect_max_usages::verify(card *origin_card, player *origin) const {
        if (origin_card->usages >= effect_value) {
            throw game_error("ERROR_MAX_USAGES", origin_card, effect_value);
        }
    }

    bool effect_max_usages::can_respond(card *origin_card, player *origin) const {
        return origin_card->usages < effect_value;
    }

    void effect_max_usages::on_play(card *origin_card, player *origin) {
        ++origin_card->usages;
    }

    void effect_pass_turn::on_play(card *origin_card, player *origin) {
        origin->pass_turn();
    }

    bool effect_resolve::can_respond(card *origin_card, player *origin) const {
        if (!origin->m_game->m_requests.empty()) {
            const auto &req = origin->m_game->top_request();
            if (origin == req.target()) {
                return req.resolvable();
            }
        }
        return false;
    }
    
    void effect_resolve::on_play(card *origin_card, player *origin) {
        origin->m_game->top_request().on_resolve();
    }

    void effect_damage::verify(card *origin_card, player *origin, player *target) const {
        if (target->m_hp <= 1) {
            throw game_error("ERROR_CANT_SELF_DAMAGE");
        }
    }

    void effect_damage::on_play(card *origin_card, player *origin, player *target) {
        target->damage(origin_card, origin, 1);
    }

    void effect_bang::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        target->m_game->queue_request<request_type::bang>(origin_card, origin, target, flags);
    }

    void effect_bangcard::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        target->m_game->queue_event<event_type::on_play_bang>(origin);
        target->m_game->queue_delayed_action([=, flags = flags]{
            request_bang req{origin_card, origin, target, flags};
            req.is_bang_card = true;
            origin->apply_bang_mods(req);
            origin->m_game->queue_request(std::move(req));
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
                origin->m_game->pop_request(request_type::bang);
            } else {
                origin->m_game->send_request_update();
            }
        } else {
            origin->m_game->pop_request(request_type::ricochet);
        }
    }

    bool effect_bangresponse::can_respond(card *origin_card, player *origin) const {
        return origin->check_player_flags(player_flags::treat_missed_as_bang)
            && effect_missed().can_respond(origin_card, origin);
    }

    void effect_bangresponse::on_play(card *origin_card, player *target) {
        effect_missed().on_play(origin_card, target);
    }

    static std::vector<card *> &barrels_used(request_holder &holder) {
        if (holder.is(request_type::bang)) {
            return holder.get<request_type::bang>().barrels_used;
        } else if (holder.is(request_type::ricochet)) {
            return holder.get<request_type::ricochet>().barrels_used;
        }
        throw std::runtime_error("Invalid request");
    };

    bool effect_barrel::can_respond(card *origin_card, player *origin) const {
        if (effect_missed().can_respond(origin_card, origin)) {
            const auto &vec = barrels_used(origin->m_game->top_request());
            return std::ranges::find(vec, origin_card) == vec.end();
        }
        return false;
    }

    void effect_barrel::on_play(card *origin_card, player *target) {
        barrels_used(target->m_game->top_request()).push_back(origin_card);
        target->m_game->send_request_update();
        target->m_game->draw_check_then(target, origin_card, [=](card *drawn_card) {
            if (target->get_card_suit(drawn_card) == card_suit_type::hearts) {
                effect_missed().on_play(origin_card, target);
            }
        });
    }

    void effect_banglimit::verify(card *origin_card, player *origin) const {
        bool value = origin->m_bangs_played < origin->m_bangs_per_turn;
        origin->m_game->instant_event<event_type::apply_volcanic_modifier>(origin, value);
        if (!value) {
            throw game_error("ERROR_ONE_BANG_PER_TURN");
        }
    }

    void effect_banglimit::on_play(card *origin_card, player *origin) {
        ++origin->m_bangs_played;
    }

    void effect_indians::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        target->m_game->queue_request<request_type::indians>(origin_card, origin, target, flags);
    }

    void effect_duel::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        target->m_game->queue_request<request_type::duel>(origin_card, origin, target, origin, flags);
    }

    void effect_generalstore::on_play(card *origin_card, player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            origin->m_game->draw_card_to(card_pile_type::selection);
        }
        origin->m_game->queue_request<request_type::generalstore>(origin_card, origin, origin);
    }

    void effect_heal::on_play(card *origin_card, player *origin, player *target) {
        target->heal(std::max(1, effect_value));
    }

    void effect_heal_notfull::verify(card *origin_card, player *origin, player *target) const {
        if (target->m_hp == target->m_max_hp) {
            throw game_error("ERROR_CANT_HEAL_PAST_FULL_HP");
        }
    }

    void effect_beer::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->queue_event<event_type::on_play_beer>(target);
        if (target->m_game->m_players.size() <= 2 || target->m_game->num_alive() > 2) {
            int amt = 1;
            target->m_game->instant_event<event_type::apply_beer_modifier>(target, amt);
            target->heal(amt);
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
            origin->m_game->pop_request(request_type::death);
        }
    }

    void effect_destroy::on_play(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags) {
        if (origin != target && target->can_escape(origin, origin_card, flags)) {
            target->m_game->queue_request<request_type::destroy>(origin_card, origin, target, target_card, flags);
        } else {
            on_resolve(origin_card, origin, target, target_card);
        }
    }

    void effect_destroy::on_resolve(card *origin_card, player *origin, player *target, card *target_card) {
        auto fun = [=]{
            if (origin->alive()) {
                if (origin != target) {
                    target->m_game->add_log("LOG_DISCARDED_CARD", origin, target, with_owner{target_card});
                } else {
                    target->m_game->add_log("LOG_DISCARDED_SELF_CARD", origin, target_card);
                }
                target->discard_card(target_card);
            }
        };
        // check henry block
        size_t nreqs = target->m_game->m_requests.size();
        target->m_game->instant_event<event_type::on_discard_card>(origin, target, target_card);
        if (target->m_game->m_requests.size() > nreqs) {
            // check suzy lafayette
            if (auto pos = std::ranges::find(target->m_game->m_pending_events, enums::indexof(event_type::on_effect_end), &event_args::index);
                pos != target->m_game->m_pending_events.end()) {
                target->m_game->m_pending_events.emplace(pos, std::in_place_index<enums::indexof(event_type::delayed_action)>, std::move(fun));
            } else {
                target->m_game->queue_delayed_action(std::move(fun));
            }
        } else {
            fun();
        }
    }

    bool effect_drawing::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is(request_type::draw, origin);
    }

    void effect_steal::on_play(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags) {
        if (origin != target && target->can_escape(origin, origin_card, flags)) {
            target->m_game->queue_request<request_type::steal>(origin_card, origin, target, target_card, flags);
        } else {
            on_resolve(origin_card, origin, target, target_card);
        }
    }

    void effect_steal::on_resolve(card *origin_card, player *origin, player *target, card *target_card) {
        auto fun = [=]{
            if (origin->alive()) {
                if (origin != target) {
                    target->m_game->add_log("LOG_STOLEN_CARD", origin, target, with_owner{target_card});
                } else {
                    target->m_game->add_log("LOG_STOLEN_SELF_CARD", origin, target_card);
                }
                origin->steal_card(target, target_card);
            }
        };
        // check henry block
        size_t nreqs = target->m_game->m_requests.size();
        target->m_game->instant_event<event_type::on_discard_card>(origin, target, target_card);
        if (target->m_game->m_requests.size() > nreqs) {
            // check suzy lafayette
            if (auto pos = std::ranges::find(target->m_game->m_pending_events, enums::indexof(event_type::on_effect_end), &event_args::index);
                pos != target->m_game->m_pending_events.end()) {
                target->m_game->m_pending_events.emplace(pos, std::in_place_index<enums::indexof(event_type::delayed_action)>, std::move(fun));
            } else {
                target->m_game->queue_delayed_action(std::move(fun));
            }
        } else {
            fun();
        }
    }

    void effect_choose_card::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->discard_card(target_card);
        target->m_game->add_log("LOG_CHOSE_CARD_FOR", origin_card, origin, target_card);
        target->m_game->add_event<event_type::apply_chosen_card_modifier>(target_card, [=](player *p, card* &c) {
            if (p == origin && c == origin_card) {
                c = target_card;
            }
        });

        target->m_game->add_event<event_type::on_effect_end>(target_card, [=](player *p, card *c) {
            if (p == origin && c == origin_card) {
                origin->m_game->remove_events(target_card);
            }
        });
    }

    void effect_draw::on_play(card *origin_card, player *origin, player *target) {
        for (int i=0; i<std::max(1, effect_value); ++i) {
            card *drawn_card = target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->add_log("LOG_DRAWN_CARD", target, drawn_card);
        }
    }

    void effect_draw_discard::verify(card *origin_card, player *origin, player *target) const {
        if (target->m_game->m_discards.empty()) {
            throw game_error("ERROR_DISCARD_PILE_EMPTY");
        }
    }

    void effect_draw_discard::on_play(card *origin_card, player *origin, player *target) {
        card *drawn_card = target->m_game->m_discards.back();
        target->m_game->add_log("LOG_DRAWN_FROM_DISCARD", target, drawn_card);
        target->add_to_hand(drawn_card);
    }

    void effect_draw_rest::on_play(card *origin_card, player *target) {
        while (target->m_num_drawn_cards < target->m_num_cards_to_draw) {
            ++target->m_num_drawn_cards;
            card *drawn_card = target->m_game->draw_phase_one_card_to(card_pile_type::player_hand, target);
            target->m_game->add_log("LOG_DRAWN_CARD", target, drawn_card);
            target->m_game->instant_event<event_type::on_card_drawn>(target, drawn_card);
        }
        if (target->m_game->pop_request(request_type::draw)) {
            target->m_game->queue_event<event_type::post_draw_cards>(target);
        }
    }

    void effect_draw_done::on_play(card *origin_card, player *target) {
        if (target->m_game->pop_request(request_type::draw)) {
            target->m_game->queue_event<event_type::post_draw_cards>(target);
        }
    }

    void effect_draw_skip::verify(card *origin_card, player *target) const {
        if (target->m_num_drawn_cards >= target->m_num_cards_to_draw) {
            throw game_error("ERROR_PLAYER_MUST_NOT_DRAW");
        }
    }

    void effect_draw_skip::on_play(card *origin_card, player *target) {
        if (++target->m_num_drawn_cards == target->m_num_cards_to_draw) {
            if (target->m_game->pop_request(request_type::draw)) {
                target->m_game->queue_event<event_type::post_draw_cards>(target);
            }
        }
    }

}