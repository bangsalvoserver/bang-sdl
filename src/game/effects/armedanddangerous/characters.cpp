#include "characters.h"
#include "requests.h"

#include "../base/requests.h"

#include "../../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_al_preacher::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_equip_card>(target_card, [=](player *origin, player *target, card *equipped_card) {
            if (p != origin && (equipped_card->color == card_color_type::blue || equipped_card->color == card_color_type::orange)) {
                if (p->count_cubes() >= 2) {
                    p->m_game->queue_request<timer_al_preacher>(target_card, origin, p);
                }
            }
        });
    }

    bool effect_al_preacher::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<timer_al_preacher>(origin);
    }

    void effect_al_preacher::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request<timer_al_preacher>();
    }

    void effect_julie_cutter::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>(target_card, [=](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin && p == target && origin != target) {
                target->m_game->queue_action([=]{
                    if (target->alive()) {
                        target->m_game->draw_check_then(target, target_card, [=](card *drawn_card) {
                            card_suit suit = target->get_card_sign(drawn_card).suit;
                            if (suit == card_suit::hearts || suit == card_suit::diamonds) {
                                target->m_game->add_log("LOG_CARD_HAS_EFFECT", target_card);
                                target->m_game->queue_request<request_bang>(target_card, target, origin);
                            }
                        });
                    }
                });
            }
        });
    }

    opt_error effect_frankie_canton::verify(card *origin_card, player *origin, card *target_card) const {
        if (target_card == origin->m_characters.front()) {
            return game_error("ERROR_INVALID_ACTION");
        }
        if (target_card->cubes.empty()) {
            return game_error("ERROR_NOT_ENOUGH_CUBES_ON", target_card);
        }
        return std::nullopt;
    }

    void effect_frankie_canton::on_play(card *origin_card, player *origin, card *target_card) {
        origin->m_game->add_log("LOG_PLAYED_CARD_ON", origin_card, origin, target_card);
        target_card->owner->move_cubes(target_card, origin->m_characters.front(), 1);
    }

    void effect_bloody_mary::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_missed>(target_card, [=](card *origin_card, player *origin, player *target, bool is_bang) {
            if (origin == p && is_bang) {
                p->draw_card(1, target_card);
            }
        });
    }

    void effect_red_ringo::on_equip(card *target_card, player *target) {
        target->add_cubes(target->m_characters.front(), 4);
    }

    opt_error effect_red_ringo::verify(card *origin_card, player *origin, card *target_card) const {
        if (origin->m_characters.front()->cubes.size() == 0) {
            return game_error("ERROR_NOT_ENOUGH_CUBES_ON", origin->m_characters.front());
        }
        if (target_card->cubes.size() >= 4) {
            return game_error("ERROR_CARD_HAS_FULL_CUBES", target_card);
        }
        return std::nullopt;
    }

    void effect_red_ringo::on_play(card *origin_card, player *origin, card *target_card) {
        origin->move_cubes(origin->m_characters.front(), target_card, 1);
    }

    bool effect_ms_abigail::can_escape(player *origin, card *origin_card, effect_flags flags) const {
        if (!origin) return false;
        origin_card = origin->chosen_card_or(origin_card);
        if (!bool(flags & effect_flags::single_target)) return false;
        if (origin_card->color != card_color_type::brown) return false;
        switch (origin->get_card_sign(origin_card).rank) {
        case card_rank::rank_J:
        case card_rank::rank_Q:
        case card_rank::rank_K:
        case card_rank::rank_A:
            return true;
        default:
            return false;
        }
    }

    bool effect_ms_abigail::can_respond(card *origin_card, player *origin) const {
        if (!origin->m_game->m_requests.empty()) {
            auto &req = origin->m_game->top_request();
            return req.target() == origin && can_escape(req.origin(), req.origin_card(), req.flags());
        }
        return false;
    }

    void effect_ms_abigail::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request();
    }

    void effect_ms_abigail::on_enable(card *origin_card, player *origin) {
        origin->m_game->add_event<event_type::apply_escapable_modifier>(origin_card,
            [=](card *e_origin_card, player *e_origin, const player *e_target, effect_flags e_flags, bool &value) {
                value = value || (e_target == origin) && effect_ms_abigail{}.can_escape(e_origin, e_origin_card, e_flags);
            });
    }
}