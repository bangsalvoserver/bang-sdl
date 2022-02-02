#include "common/effects/effects_base.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

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
        enums::visit_indexed([]<request_type E>(enums::enum_constant<E>, auto &req) {
            if constexpr (resolvable_request<E>) {
                auto req_copy = std::move(req);
                req_copy.on_resolve();
            }
        }, origin->m_game->top_request());
    }

    bool effect_predraw::can_respond(card *origin_card, player *origin) const {
        if (origin->m_game->top_request_is(request_type::predraw, origin)) {
            int top_priority = std::ranges::max(origin->m_predraw_checks
                | std::views::values
                | std::views::filter(std::not_fn(&player::predraw_check::resolved))
                | std::views::transform(&player::predraw_check::priority));
            auto it = origin->m_predraw_checks.find(origin_card);
            if (it != origin->m_predraw_checks.end()
                && !it->second.resolved
                && it->second.priority == top_priority) {
                return true;
            }
        }
        return false;
    }
    
    void effect_predraw::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request(request_type::predraw);
        origin->m_game->draw_check_then(origin, origin_card, origin->m_predraw_checks.find(origin_card)->second.check_fun);
    }

    void effect_damage::verify(card *origin_card, player *origin, player *target) const {
        if (target->m_hp <= 1) {
            throw game_error("ERROR_CANT_SELF_DAMAGE");
        }
    }

    void effect_damage::on_play(card *origin_card, player *origin, player *target) {
        target->damage(origin_card, origin, 1);
    }

    void effect_bang::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        target->m_game->queue_request<request_type::bang>(origin_card, origin, target, flags);
    }

    void effect_bangcard::verify(card *origin_card, player *origin, player *target) const {
        if (origin->m_game->has_scenario(scenario_flags::sermon)) {
            throw game_error("ERROR_SCENARIO_AT_PLAY", origin->m_game->m_scenario_cards.back());
        }
    }

    void effect_bangcard::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        target->m_game->queue_event<event_type::on_play_bang>(origin);
        target->m_game->queue_event<event_type::delayed_action>([=, flags = flags]{
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
                origin->m_game->send_request_respond();
            }
        } else {
            origin->m_game->pop_request(request_type::ricochet);
        }
    }

    void effect_missedcard::verify(card *origin_card, player *origin) const {
        origin->m_game->instant_event<event_type::verify_missedcard>(origin, origin_card);
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
        target->m_game->send_request_respond();
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
        bool immune = false;
        target->m_game->instant_event<event_type::apply_indianguide_modifier>(target, immune);
        if (!immune) {
            target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
            target->m_game->queue_request<request_type::indians>(origin_card, origin, target, flags);
        }
    }

    void effect_duel::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
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
            target->m_game->pop_request_noupdate(request_type::duel);
            target->m_game->queue_request<request_type::duel>(origin_card, origin, respond_to, target);
            break;
        }
        case request_type::indians:
            target->m_game->pop_request(request_type::indians);
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
        target->heal(std::max<short>(1, args));
    }

    void effect_heal_notfull::verify(card *origin_card, player *origin, player *target) const {
        if (target->m_hp == target->m_max_hp) {
            throw game_error("ERROR_CANT_HEAL_PAST_FULL_HP");
        }
    }

    void effect_beer::verify(card *origin_card, player *origin, player *target) const {
        if (origin->m_game->has_scenario(scenario_flags::reverend)) {
            throw game_error("ERROR_SCENARIO_AT_PLAY", origin->m_game->m_scenario_cards.back());
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

    void effect_destroy::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        if (origin != target && target->can_escape(origin, origin_card, flags)) {
            target->m_game->queue_request<request_type::destroy>(origin_card, origin, target, target_card, flags);
        } else {
            target->m_game->instant_event<event_type::on_discard_card>(origin, target, target_card);
            auto fun = [=]{
                if (origin != target) {
                    target->m_game->add_log("LOG_DISCARDED_CARD", origin, target, with_owner{target_card});
                } else {
                    target->m_game->add_log("LOG_DISCARDED_SELF_CARD", origin, target_card);
                }
                target->discard_card(target_card);
            };
            if (auto pos = std::ranges::find(target->m_game->m_pending_events, enums::indexof(event_type::on_effect_end), &event_args::index);
                pos != target->m_game->m_pending_events.end()) {
                target->m_game->m_pending_events.emplace(pos, std::in_place_index<enums::indexof(event_type::delayed_action)>, std::move(fun));
            } else {
                target->m_game->queue_event<event_type::delayed_action>(std::move(fun));
            }
        }
    }

    bool effect_drawing::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is(request_type::draw);
    }

    void effect_steal::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        if (origin != target && target->can_escape(origin, origin_card, flags)) {
            target->m_game->queue_request<request_type::steal>(origin_card, origin, target, target_card, flags);
        } else {
            target->m_game->instant_event<event_type::on_discard_card>(origin, target, target_card);
            auto fun = [=]{
                if (origin != target) {
                    target->m_game->add_log("LOG_STOLEN_CARD", origin, target, with_owner{target_card});
                } else {
                    target->m_game->add_log("LOG_STOLEN_SELF_CARD", origin, target_card);
                }
                origin->steal_card(target, target_card);
            };
            if (auto pos = std::ranges::find(target->m_game->m_pending_events, enums::indexof(event_type::on_effect_end), &event_args::index);
                pos != target->m_game->m_pending_events.end()) {
                target->m_game->m_pending_events.emplace(pos, std::in_place_index<enums::indexof(event_type::delayed_action)>, std::move(fun));
            } else {
                target->m_game->queue_event<event_type::delayed_action>(std::move(fun));
            }
        }
    }

    void effect_choose_card::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->discard_card(target_card);
        target->m_game->add_log("LOG_CHOSE_CARD_FOR", origin_card, origin, target_card);
        target->m_chosen_card = target_card;

        target->m_game->add_event<event_type::on_play_card_end>(target_card, [=](player *p, card *c) {
            if (p == origin && c == origin_card) {
                origin->m_game->queue_event<event_type::delayed_action>([=]{
                    origin->m_chosen_card = nullptr;
                });
                origin->m_game->remove_events(target_card);
            }
        });
    }

    void effect_draw::on_play(card *origin_card, player *origin, player *target) {
        card *drawn_card = target->m_game->draw_card_to(card_pile_type::player_hand, target);
        target->m_game->add_log("LOG_DRAWN_CARD", target, drawn_card);
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
        target->m_game->pop_request(request_type::draw);
        target->m_game->queue_event<event_type::post_draw_cards>(target);
    }

    void effect_draw_done::on_play(card *origin_card, player *target) {
        target->m_game->pop_request(request_type::draw);
        target->m_game->queue_event<event_type::post_draw_cards>(target);
    }

    void effect_draw_skip::verify(card *origin_card, player *target) const {
        if (target->m_num_drawn_cards >= target->m_num_cards_to_draw) {
            throw game_error("ERROR_PLAYER_MUST_NOT_DRAW");
        }
    }

    void effect_draw_skip::on_play(card *origin_card, player *target) {
        if (++target->m_num_drawn_cards == target->m_num_cards_to_draw) {
            target->m_game->pop_request(request_type::draw);
            target->m_game->queue_event<event_type::post_draw_cards>(target);
        }
    }

}