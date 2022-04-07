#include "effects_valleyofshadows.h"
#include "requests_valleyofshadows.h"
#include "requests_base.h"
#include "effects_base.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_aim::on_play(card *origin_card, player *origin) {
        origin->m_game->add_single_call_event<event_type::apply_bang_modifier>(origin_card, [=](player *p, request_bang *req) {
            if (p == origin) {
                ++req->bang_damage;
                return true;
            }
            return false;
        });
    }
    
    void effect_backfire::verify(card *origin_card, player *origin) const {
        if (origin->m_game->m_requests.empty() || !origin->m_game->top_request().origin()) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_backfire::on_play(card *origin_card, player *origin) {
        origin->m_game->queue_request<request_bang>(origin_card, origin, origin->m_game->top_request().origin(),
            effect_flags::escapable | effect_flags::single_target);
    }

    void effect_bandidos::on_play(card *origin_card, player *origin, player *target) {
        target->m_game->queue_request<request_bandidos>(origin_card, origin, target, effect_flags::escapable);
    }

    void effect_tornado::on_play(card *origin_card, player *origin, player *target) {
        if (target->m_hand.empty()) {
            target->m_game->queue_action([=]{
                target->m_game->draw_card_to(card_pile_type::player_hand, target);
                target->m_game->draw_card_to(card_pile_type::player_hand, target);
            });
        } else {
            target->m_game->queue_request<request_tornado>(origin_card, origin, target);
        }
    }

    void effect_poker::on_play(card *origin_card, player *origin) {
        auto target = origin;
        effect_flags flags = effect_flags::escapable;
        if (std::ranges::count_if(origin->m_game->m_players, [&](const player &p) {
            return &p != origin && p.alive() && !p.m_hand.empty();
        }) == 1) {
            flags |= effect_flags::single_target;
        }
        while(true) {
            target = origin->m_game->get_next_player(target);
            if (target == origin) break;
            if (!target->m_hand.empty()) {
                origin->m_game->queue_request<request_poker>(origin_card, origin, target, flags);
            }
        };
        origin->m_game->queue_action([=]{
            for (auto it = origin->m_game->m_selection.begin(); it != origin->m_game->m_selection.end(); ++it) {
                auto flags = show_card_flags::show_everyone;
                if (std::next(it) == origin->m_game->m_selection.end()) {
                    flags |= show_card_flags::short_pause;
                }
                origin->m_game->send_card_update(**it, nullptr, flags);
            }
            if (std::ranges::find(origin->m_game->m_selection, card_value_type::value_A, &card::value) != origin->m_game->m_selection.end()) {
                while (!target->m_game->m_selection.empty()) {
                    origin->m_game->move_to(target->m_game->m_selection.front(), card_pile_type::discard_pile);
                }
            } else if (origin->m_game->m_selection.size() <= 2) {
                while (!origin->m_game->m_selection.empty()) {
                    origin->add_to_hand(origin->m_game->m_selection.front());
                }
            } else {
                origin->m_game->queue_request<request_poker_draw>(origin_card, origin);
            }
        });
    }

    bool effect_saved::can_respond(card *origin_card, player *origin) const {
        if (auto *req = origin->m_game->top_request_if<timer_damaging>()) {
            return req->target != origin;
        }
        return false;
    }

    void effect_saved::on_play(card *origin_card, player *origin) {
        auto &req = origin->m_game->top_request().get<timer_damaging>();
        player *saved = req.target;
        if (0 == --req.damage) {
            origin->m_game->pop_request<timer_damaging>();
        }
        origin->m_game->queue_action([=]{
            if (saved->alive()) {
                origin->m_game->queue_request<request_saved>(origin_card, origin, saved);
            }
        });
    }

    bool effect_escape::can_respond(card *origin_card, player *origin) const {
        return !origin->m_game->m_requests.empty() && origin->m_game->top_request().target() == origin
            && bool(origin->m_game->top_request().flags() & effect_flags::escapable);
    }

    void effect_escape::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request();
    }

    void handler_fanning::verify(card *origin_card, player *origin, const mth_target_list &targets) const {
        player *target_players[] = {
            std::get<player *>(targets[0]),
            std::get<player *>(targets[1])
        };
        if (origin->m_game->calc_distance(target_players[0], target_players[1]) > 1
            && target_players[0] != target_players[1])
        {
            throw game_error("ERROR_TARGET_NOT_IN_RANGE");
        }
    }

    void handler_fanning::on_play(card *origin_card, player *origin, const mth_target_list &targets) {
        for (auto [target, _] : targets) {
            effect_bang{}.on_play(origin_card, origin, target, effect_flags::escapable);
        }
    }

}