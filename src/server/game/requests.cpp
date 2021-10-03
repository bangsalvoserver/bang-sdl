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
                target->m_game->pop_request();
                for (auto &e : c.equips) {
                    e.on_predraw_check(target, card_id);
                }
            }
        }
    }

    void request_check::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            target->m_game->pop_request();
            target->m_game->resolve_check(card_id);
        }
    }

    void request_generalstore::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto next = target->m_game->get_next_player(target);
            auto removed = target->m_game->draw_from_temp(card_id);
            if (target->m_game->m_temps.size() == 1) {
                target->m_game->pop_request();
                target->add_to_hand(std::move(removed));
                next->add_to_hand(std::move(target->m_game->m_temps.front()));
                target->m_game->m_temps.clear();
            } else {
                target->m_game->pop_request_noupdate();
                target->add_to_hand(std::move(removed));
                target->m_game->queue_request<request_type::generalstore>(origin, next);
            }
        }
    }

    void request_discard::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            target->m_game->pop_request();
            target->discard_card(card_id);
            if (target->num_hand_cards() <= target->m_hp) {
                target->m_game->next_turn();
            }
        }
    }

    void request_damaging::on_resolve() {
        target->m_game->pop_request();
        target->damage(origin, 1);
    }

    void request_death::on_resolve() {
        target->m_game->player_death(origin, target);
        target->m_game->pop_request();
    }
}