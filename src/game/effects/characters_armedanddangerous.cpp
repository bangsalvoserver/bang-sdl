#include "characters_armedanddangerous.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_al_preacher::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_equip>(target_card, [=](player *origin, player *target, card *equipped_card) {
            if (p != origin && (equipped_card->color == card_color_type::blue || equipped_card->color == card_color_type::orange)) {
                if (p->count_cubes() >= 2) {
                    p->m_game->queue_request<request_type::al_preacher>(target_card, origin, p);
                }
            }
        });
    }

    bool effect_al_preacher::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is(request_type::al_preacher, origin);
    }

    void effect_al_preacher::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request(request_type::al_preacher);
    }

    void effect_julie_cutter::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>(target_card, [=](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin && p == target && origin != target) {
                p->m_game->draw_check_then(target, target_card, [=](card *drawn_card) {
                    card_suit_type suit = p->get_card_suit(drawn_card);
                    if (suit == card_suit_type::hearts || suit == card_suit_type::diamonds) {
                        p->m_game->queue_request<request_type::bang>(target_card, target, origin);
                    }
                });
            }
        });
    }

    void effect_frankie_canton::verify(card *origin_card, player *origin, player *target, card *target_card) const {
        if (target_card == origin->m_characters.front()) throw game_error("ERROR_INVALID_ACTION");
        if (target_card->cubes.empty()) throw game_error("ERROR_NOT_ENOUGH_CUBES_ON", target_card);
    }

    void effect_frankie_canton::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->move_cubes(target_card, origin->m_characters.front(), 1);
    }

    void effect_bloody_mary::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_missed>(target_card, [=](card *origin_card, player *origin, player *target, bool is_bang) {
            if (origin == p && is_bang) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, p);
            }
        });
    }

    void effect_red_ringo::on_pre_equip(card *target_card, player *target) {
        target->add_cubes(target->m_characters.front(), 4);
    }

    void effect_red_ringo::verify(card *origin_card, player *origin, player *target, card *target_card) const {
        if (origin->m_characters.front()->cubes.size() == 0) throw game_error("ERROR_NOT_ENOUGH_CUBES_ON", origin->m_characters.front());
        if (target_card->cubes.size() >= 4) throw game_error("ERROR_CARD_HAS_FULL_CUBES", target_card);
    }

    void effect_red_ringo::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        origin->move_cubes(origin->m_characters.front(), target_card, 1);
    }

    bool effect_ms_abigail::can_escape(player *origin, card *origin_card, effect_flags flags) const {
        if (!origin) return false;
        if (origin->m_chosen_card) origin_card = origin->m_chosen_card;
        if (!bool(flags & effect_flags::single_target)) return false;
        if (origin_card->color != card_color_type::brown) return false;
        switch (origin->get_card_value(origin_card)) {
        case card_value_type::value_J:
        case card_value_type::value_Q:
        case card_value_type::value_K:
        case card_value_type::value_A:
            return true;
        default:
            return false;
        }
    }

    bool effect_ms_abigail::can_respond(card *origin_card, player *origin) const {
        if (!origin->m_game->m_requests.empty()) {
            auto &req = origin->m_game->top_request();
            return can_escape(req.origin(), req.origin_card(), req.flags());
        }
        return false;
    }

    void effect_ms_abigail::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request();
    }
}