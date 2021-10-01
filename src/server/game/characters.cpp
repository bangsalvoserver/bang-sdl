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
            t->add_to_hand(t->m_game->draw_from_temp(card_id));
            if (t->m_game->m_temps.size() == 1) {
                t->m_game->pop_response();
                auto removed = std::move(t->m_game->m_temps.front());
                t->m_game->add_private_update<game_update_type::hide_card>(target, removed.id);
                t->m_game->add_public_update<game_update_type::move_card>(removed.id, 0, card_pile_type::main_deck);
                t->m_game->m_temps.clear();
                t->m_game->m_deck.push_back(std::move(removed));
            } else {
                t->m_game->pop_response_noupdate();
                t->m_game->queue_response<response_type::kit_carlson>(nullptr, target);
            }
        }
    }

    void effect_claus_the_saint::on_play(player *target) {
        int ncards = target->m_game->num_alive() + target->m_num_drawn_cards - 1;
        for (int i=0; i<ncards; ++i) {
            target->m_game->add_to_temps(target->m_game->draw_card(), target);
        }
        target->m_game->queue_response<response_type::claus_the_saint>(nullptr, target);
    }

    void response_claus_the_saint::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto t = target;
            int index = target->m_game->num_alive() + target->m_num_drawn_cards - target->m_game->m_temps.size();
            auto p = t;
            for(int i=0; i<index; ++i) {
                p = t->m_game->get_next_player(p);
            }
            p->add_to_hand(t->m_game->draw_from_temp(card_id));
            if (t->m_game->m_temps.size() == target->m_num_drawn_cards) {
                t->m_game->pop_response();
                for (auto &c : t->m_game->m_temps) {
                    t->add_to_hand(std::move(c));
                }
                t->m_game->m_temps.clear();
            } else {
                t->m_game->pop_response_noupdate();
                t->m_game->queue_response<response_type::claus_the_saint>(nullptr, target);
            }
        }
    }

    void effect_el_gringo::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target) {
            if (p == target && !origin->m_hand.empty()) {
                target->steal_card(origin, origin->random_hand_card().id);
            }
        });
    }

    void effect_el_gringo::on_unequip(player *target, int card_id) {
        target->m_game->remove_event(card_id);
    }

    void effect_suzy_lafayette::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_empty_hand>(card_id, [p](player *target) {
            if (p == target && p->m_hand.empty()) {
                target->add_to_hand(target->m_game->draw_card());
            }
        });
    }

    void effect_suzy_lafayette::on_unequip(player *target, int card_id) {
        target->m_game->remove_event(card_id);
    }

    void effect_vulture_sam::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_player_death>(card_id, [p](player *origin, player *target) {
            for (auto &c : target->m_table) {
                p->add_to_hand(std::move(c));
            }
            target->m_table.clear();
            for (auto &c : target->m_hand) {
                p->add_to_hand(std::move(c));
            }
            target->m_hand.clear();
        });
    }

    void effect_vulture_sam::on_unequip(player *target, int card_id) {
        target->m_game->remove_event(card_id);
    }

    void effect_greg_digger::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_player_death>(card_id, [p](player *origin, player *target) {
            p->heal(2);
        });
    }

    void effect_greg_digger::on_unequip(player *target, int card_id) {
        target->m_game->remove_event(card_id);
    }

    void effect_herb_hunter::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_player_death>(card_id, [p](player *origin, player *target) {
            p->add_to_hand(p->m_game->draw_card());
            p->add_to_hand(p->m_game->draw_card());
        });
    }

    void effect_herb_hunter::on_unequip(player *target, int card_id) {
        target->m_game->remove_event(card_id);
    }

    void effect_johnny_kisch::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_equip>(card_id, [p](player *target, int equipped_card) {
            if (p == target) {
                const auto &name = p->find_table_card(equipped_card).name;
                for (auto &other : p->m_game->m_players) {
                    if (other.id == p->id) continue;
                    auto it = std::ranges::find(other.m_table, name, &deck_card::name);
                    if (it != other.m_table.end()) {
                        other.discard_card(it->id);
                    }
                }
            }
        });
    }

    void effect_johnny_kisch::on_unequip(player *target, int card_id) {
        target->m_game->remove_event(card_id);
    }

    void effect_molly_stark::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_play_off_turn>(card_id, [p](player *target, int player_card) {
            if (p == target) {
                p->add_to_hand(p->m_game->draw_card());
            }
        });
    }

    void effect_molly_stark::on_unequip(player *target, int card_id) {
        target->m_game->remove_event(card_id);
    }

    void effect_bellestar::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_turn_start>(card_id, [p](player *target) {
            if (p == target) {
                p->m_game->disable_table_cards(p->id);
            }
        });
        p->m_game->add_event<event_type::on_turn_end>(card_id, [p](player *target) {
            if (p == target) {
                p->m_game->enable_table_cards(p->id);
            }
        });
    }

    void effect_bellestar::on_unequip(player *target, int card_id) {
        target->m_game->enable_table_cards(target->id);
        target->m_game->remove_event(card_id);
    }
}