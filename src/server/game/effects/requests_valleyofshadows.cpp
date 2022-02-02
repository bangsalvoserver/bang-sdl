#include "common/effects/requests_valleyofshadows.h"

#include "../game.h"

namespace banggame {
    
    void timer_damaging::on_finished() {
        if (origin->m_hp <= damage) {
            origin->m_game->pop_request_noupdate(request_type::damaging);
        } else {
            origin->m_game->pop_request(request_type::damaging);
        }
        origin->do_damage(origin_card, source, damage, is_bang);
        cleanup();
    }

    void timer_damaging::cleanup() {
        if (cleanup_function) {
            cleanup_function();
            cleanup_function = nullptr;
        }
    }

    game_formatted_string timer_damaging::status_text() const {
        return {damage > 1 ? "STATUS_DAMAGING_PLURAL" : "STATUS_DAMAGING", origin, origin_card, damage};
    }

    void request_destroy::on_resolve() {
        effect_destroy{}.on_play(origin_card, origin, target, target_card);
        target->m_game->pop_request();
    }

    game_formatted_string request_destroy::status_text() const {
        return {"STATUS_DESTROY", origin_card, with_owner{target_card}};
    }

    void request_steal::on_resolve() {
        effect_steal{}.on_play(origin_card, origin, target, target_card);
        target->m_game->pop_request(request_type::steal);
    }

    game_formatted_string request_steal::status_text() const {
        return {"STATUS_STEAL", origin_card, with_owner{target_card}};
    }

    void request_bandidos::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            target->discard_card(target_card);
            if (--target->m_game->top_request().get<request_type::bandidos>().num_cards == 0
                || target->num_hand_cards() == 0) {
                target->m_game->pop_request(request_type::bandidos);
            }
        }
    }

    void request_bandidos::on_resolve() {
        target->m_game->pop_request(request_type::bandidos);
        target->damage(origin_card, origin, 1);
    }

    game_formatted_string request_bandidos::status_text() const {
        return {"STATUS_BANDIDOS", origin_card};
    }

    void request_tornado::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_player == target) {
            target->discard_card(target_card);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->pop_request(request_type::tornado);
        }
    }

    game_formatted_string request_tornado::status_text() const {
        return {"STATUS_TORNADO", origin_card};
    }

    void request_poker::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target == target_player) {
            target->m_game->move_to(target_card, card_pile_type::selection, true, origin);
            target->m_game->pop_request(request_type::poker);
        }
    }

    game_formatted_string request_poker::status_text() const {
        return {"STATUS_POKER", origin_card};
    }

    void request_poker_draw::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->add_to_hand(target_card);
        if (--target->m_game->top_request().get<request_type::poker_draw>().num_cards == 0
            || target->m_game->m_selection.size() == 0) {
            for (auto *c : target->m_game->m_selection) {
                target->m_game->move_to(c, card_pile_type::discard_pile);
            }
            target->m_game->pop_request(request_type::poker_draw);
        }
    }

    game_formatted_string request_poker_draw::status_text() const {
        return {"STATUS_POKER_DRAW", origin_card};
    }

    void request_saved::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (pile == card_pile_type::main_deck) {
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
            target->m_game->pop_request(request_type::saved);
        } else if (pile == card_pile_type::player_hand && target_player == saved) {
            for (int i=0; i<2 && !saved->m_hand.empty(); ++i) {
                target->steal_card(saved, saved->random_hand_card());
            }
            target->m_game->pop_request(request_type::saved);
        }
    }

    game_formatted_string request_saved::status_text() const {
        return {"STATUS_SAVED", origin_card, saved};
    }

    game_formatted_string timer_lemonade_jim::status_text() const {
        return {"STATUS_CAN_PLAY_CARD", origin_card};
    }
}