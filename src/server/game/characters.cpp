#include "common/characters.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void effect_slab_the_killer::on_equip(player *target, int card_id) {
        ++target->m_bang_strength;
    }

    void effect_slab_the_killer::on_unequip(player *target, int card_id) {
        --target->m_bang_strength;
    }

    void effect_black_jack::on_play(player *target) {
        int ncards = target->m_num_drawn_cards;
        for (int i=0; i<ncards; ++i) {
            if (i==1) {
                auto removed = target->m_game->draw_card();
                if (removed.suit == card_suit_type::hearts || removed.suit == card_suit_type::diamonds) {
                    ++ncards;
                }
                target->m_game->add_show_card(removed);
                target->add_to_hand(std::move(removed));
            } else {
                target->add_to_hand(target->m_game->draw_card());
            }
        }
    }

    void effect_bill_noface::on_play(player *target) {
        int ncards = target->m_num_drawn_cards - 1 + target->m_max_hp - target->m_hp;
        for (int i=0; i<ncards; ++i) {
            target->add_to_hand(target->m_game->draw_card());
        }
    }

    void effect_tequila_joe::on_equip(player *target, int card_id) {
        ++target->m_beer_strength;
    }

    void effect_tequila_joe::on_unequip(player *target, int card_id) {
        --target->m_beer_strength;
    }

    void effect_sean_mallory::on_equip(player *target, int card_id) {
        target->m_max_cards_mods.push_back(10);
    }

    void effect_sean_mallory::on_unequip(player *target, int card_id) {
        target->m_max_cards_mods.erase(std::ranges::find(target->m_max_cards_mods, 10));
    }

    void effect_kit_carlson::on_play(player *target) {
        for (int i=0; i<=target->m_num_drawn_cards; ++i) {
            target->m_game->add_to_temps(target->m_game->draw_card(), target);
        }
        target->m_game->queue_response<response_type::kit_carlson>(nullptr, target);
    }

    void response_kit_carlson::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto t = target;
            t->m_game->pop_response();
            t->add_to_hand(t->m_game->draw_from_temp(card_id));
            if (t->m_game->m_temps.size() == 1) {
                auto removed = std::move(t->m_game->m_temps.front());
                t->m_game->add_private_update<game_update_type::hide_card>(target, removed.id);
                t->m_game->add_public_update<game_update_type::move_card>(removed.id, 0, card_pile_type::main_deck);
                t->m_game->m_temps.clear();
                t->m_game->m_deck.push_back(std::move(removed));
            } else {
                t->m_game->queue_response<response_type::kit_carlson>(nullptr, target);
            }
        }
    }
}