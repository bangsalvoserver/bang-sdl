#include "effects_base.h"
#include "requests_base.h"
#include "requests_valleyofshadows.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_play_card_action::on_play(card *origin_card, player *origin) {
        origin->play_card_action(origin_card);
    }

    void effect_max_usages::verify(card *origin_card, player *origin) const {
        if (origin_card->usages >= max_usages) {
            throw game_error("ERROR_MAX_USAGES", origin_card, max_usages);
        }
    }

    bool effect_max_usages::can_respond(card *origin_card, player *origin) const {
        return origin_card->usages < max_usages;
    }

    void effect_max_usages::on_play(card *origin_card, player *origin) {
        ++origin_card->usages;
    }

    void effect_pass_turn::verify(card *origin_card, player *origin) const {
        origin->verify_pass_turn();
    }

    void effect_pass_turn::on_play(card *origin_card, player *origin) {
        origin->pass_turn();
    }

    bool effect_resolve::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<resolvable_request>(origin);
    }
    
    void effect_resolve::on_play(card *origin_card, player *origin) {
        auto copy = origin->m_game->top_request();
        copy.get<resolvable_request>().on_resolve();
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
        target->m_game->queue_request<request_bang>(origin_card, origin, target, flags);
    }

    void effect_bangcard::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        auto req = std::make_shared<request_bang>(origin_card, origin, target, flags);
        req->is_bang_card = true;
        origin->m_game->call_event<event_type::apply_bang_modifier>(origin, req.get());
        target->m_game->queue_action([origin, req = std::move(req)]() mutable {
            origin->m_game->queue_request(std::move(req));
        });
    }

    bool effect_missed::can_respond(card *origin_card, player *origin) const {
        if (auto *req = origin->m_game->top_request_if<missable_request>(origin)) {
            return req->can_respond(origin_card);
        }
        return false;
    }

    void effect_missed::on_play(card *origin_card, player *origin) {
        origin->m_game->top_request().get<missable_request>().on_miss(origin);
    }

    bool effect_bangresponse::can_respond(card *origin_card, player *origin) const {
        return origin->check_player_flags(player_flags::treat_missed_as_bang)
            && effect_missed().can_respond(origin_card, origin);
    }

    void effect_bangresponse::on_play(card *origin_card, player *target) {
        effect_missed().on_play(origin_card, target);
    }

    void effect_barrel::on_play(card *origin_card, player *target) {
        target->m_game->top_request().get<missable_request>().add_card(origin_card);
        target->m_game->update_request();
        target->m_game->draw_check_then(target, origin_card, [=](card *drawn_card) {
            if (target->get_card_sign(drawn_card).suit == card_suit_type::hearts) {
                effect_missed().on_play(origin_card, target);
            }
        });
    }

    void effect_banglimit::verify(card *origin_card, player *origin) const {
        bool value = origin->m_bangs_played < origin->m_bangs_per_turn;
        origin->m_game->call_event<event_type::apply_volcanic_modifier>(origin, value);
        if (!value) {
            throw game_error("ERROR_ONE_BANG_PER_TURN");
        }
    }

    void effect_banglimit::on_play(card *origin_card, player *origin) {
        ++origin->m_bangs_played;
    }

    void effect_indians::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        target->m_game->queue_request<request_indians>(origin_card, origin, target, flags);
    }

    void effect_duel::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        target->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target);
        target->m_game->queue_request<request_duel>(origin_card, origin, target, origin, flags);
    }

    void effect_generalstore::on_play(card *origin_card, player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            origin->m_game->draw_card_to(card_pile_type::selection);
        }
        origin->m_game->queue_request<request_generalstore>(origin_card, origin, origin);
    }

    void effect_heal::on_play(card *origin_card, player *origin, player *target) {
        target->heal(amount);
    }

    void effect_heal_notfull::verify(card *origin_card, player *origin, player *target) const {
        if (target->m_hp == target->m_max_hp) {
            throw game_error("ERROR_CANT_HEAL_PAST_FULL_HP");
        }
    }

    void effect_beer::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->call_event<event_type::on_play_beer>(target);
        if (target->m_game->m_players.size() <= 2 || target->m_game->num_alive() > 2) {
            int amt = 1;
            target->m_game->call_event<event_type::apply_beer_modifier>(target, amt);
            target->heal(amt);
        }
    }

    bool effect_deathsave::can_respond(card *origin_card, player *origin) const {
        if (auto *req = origin->m_game->top_request_if<request_death>(origin)) {
            return req->draw_attempts.empty();
        }
        return false;
    }

    void effect_deathsave::on_play(card *origin_card, player *origin) {
        if (origin->m_hp > 0) {
            origin->m_game->pop_request<request_death>();
        }
    }

    void effect_destroy::on_play(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags) {
        if (origin != target && target->can_escape(origin, origin_card, flags)) {
            target->m_game->queue_request<request_destroy>(origin_card, origin, target, target_card, flags);
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
        if (target->m_game->num_queued_requests([&]{
            target->m_game->call_event<event_type::on_discard_card>(origin, target, target_card);
        })) {
            target->m_game->queue_action(std::move(fun));
        } else {
            fun();
        }
    }

    bool effect_while_drawing::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<request_draw>(origin);
    }

    void effect_drawing::on_play(card *origin_card, player *origin) {
        if (origin->m_game->pop_request<request_draw>()) {
            origin->m_game->add_event<event_type::on_effect_end>(origin_card, [=](player *p, card *c) {
                if (p == origin && c == origin_card) {
                    origin->m_game->queue_action([=]{
                        origin->m_game->call_event<event_type::post_draw_cards>(origin);
                    });
                    origin->m_game->remove_events(origin_card);
                }
            });
        }
    }

    void effect_steal::on_play(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags) {
        if (origin != target && target->can_escape(origin, origin_card, flags)) {
            target->m_game->queue_request<request_steal>(origin_card, origin, target, target_card, flags);
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
        if (target->m_game->num_queued_requests([&]{
            target->m_game->call_event<event_type::on_discard_card>(origin, target, target_card);
        })) {
            target->m_game->queue_action(std::move(fun));
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
        for (int i=0; i<ncards; ++i) {
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

    void effect_draw_to_discard::on_play(card *origin_card, player *origin) {
        for (int i=0; i<ncards; ++i) {
            origin->m_game->draw_card_to(card_pile_type::discard_pile);
        }
    }

    void effect_draw_one_less::on_play(card *origin_card, player *target) {
        target->m_game->queue_action([=]{
            ++target->m_num_drawn_cards;
            while (target->m_num_drawn_cards++ < target->m_num_cards_to_draw) {
                card *drawn_card = target->m_game->draw_phase_one_card_to(card_pile_type::player_hand, target);
                target->m_game->add_log("LOG_DRAWN_CARD", target, drawn_card);
                target->m_game->call_event<event_type::on_card_drawn>(target, drawn_card);
            }
        });
    }

}