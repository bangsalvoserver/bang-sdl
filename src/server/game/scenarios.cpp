#include "common/scenarios.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void effect_blessing::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::apply_check_modifier>(target_card, [](card_suit_type &suit, card_value_type &value) {
            suit = card_suit_type::hearts;
        });
    }

    void effect_curse::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::apply_check_modifier>(target_card, [](card_suit_type &suit, card_value_type &value) {
            suit = card_suit_type::spades;
        });
    }

    void effect_thedaltons::on_equip(player *target, card *target_card) {
        player *p = target;
        while(true) {
            if (std::ranges::find(p->m_table, card_color_type::blue, &card::color) != p->m_table.end()) {
                p->m_game->queue_request<request_type::thedaltons>(target_card, nullptr, p);
            }

            p = target->m_game->get_next_player(p);
            if (p == target) break;
        }
    }

    void request_thedaltons::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_card->color == card_color_type::blue && target == target_player) {
            target->discard_card(target_card);
            target->m_game->pop_request();
        }
    }

    void effect_thedoctor::on_equip(player *target, card *target_card) {
        int min_hp = std::ranges::min(target->m_game->m_players
            | std::views::filter(&player::alive)
            | std::views::transform(&player::m_hp));
        
        for (auto &p : target->m_game->m_players) {
            if (p.m_hp == min_hp) {
                p.heal(1);
            }
        }
    }

    void effect_trainarrival::on_equip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            ++p.m_num_cards_to_draw;
        }
    }

    void effect_trainarrival::on_unequip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            --p.m_num_cards_to_draw;
        }
    }

    void effect_thirst::on_equip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            --p.m_num_cards_to_draw;
        }
    }

    void effect_thirst::on_unequip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            ++p.m_num_cards_to_draw;
        }
    }

    void effect_highnoon::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::on_turn_start>(target_card, [=](player *p) {
            p->damage(target_card, nullptr, 1);
        });
    }
}