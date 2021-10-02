#include "common/requests.h"
#include "common/effects.h"
#include "common/net_enums.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void request_predraw::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_table) {
            auto &c = target->find_table_card(card_id);
            if (target->is_top_predraw_check(c)) {
                auto t = target;
                t->m_game->pop_request();
                for (auto &e : c.effects) {
                    e->on_predraw_check(t, card_id);
                }
            }
        }
    }

    void request_check::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto t = target;
            t->m_game->pop_request();
            t->m_game->resolve_check(card_id);
        }
    }

    void request_generalstore::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto o = origin;
            auto t = target;
            auto next = t->m_game->get_next_player(t);
            auto removed = t->m_game->draw_from_temp(card_id);
            if (t->m_game->m_temps.size() == 1) {
                t->m_game->pop_request();
                t->add_to_hand(std::move(removed));
                next->add_to_hand(std::move(t->m_game->m_temps.front()));
                t->m_game->m_temps.clear();
            } else {
                t->m_game->pop_request_noupdate();
                t->add_to_hand(std::move(removed));
                t->m_game->queue_request<request_type::generalstore>(o, next);
            }
        }
    }

    void request_discard::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto t = target;
            t->m_game->pop_request();
            t->discard_card(card_id);
            if (t->num_hand_cards() <= t->m_hp) {
                t->m_game->next_turn();
            }
        }
    }
    
    void request_duel::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto &target_card = target->find_hand_card(card_id);
            if (target->is_bang_card(target_card)) {
                auto o = origin;
                auto t = target;
                t->m_game->pop_request_noupdate();
                t->m_game->queue_request<request_type::duel>(t, o);
                t->discard_hand_card_response(card_id);
            }
        }
    }

    void request_duel::on_resolve() {
        target->damage(origin, 1);
    }

    void request_indians::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto t = target;
            auto &target_card = t->find_hand_card(card_id);
            if (target->is_bang_card(target_card)) {
                t->discard_hand_card_response(card_id);
                t->m_game->pop_request();
            }
        }
    }

    void request_indians::on_resolve() {
        target->damage(origin, 1);
    }

    void request_bang::on_resolve() {
        target->damage(origin, 1);
    }

    void request_death::on_resolve() {
        target->handle_death();
        target->m_game->player_death(origin, target);
    }
}