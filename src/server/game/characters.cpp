#include "common/characters.h"
#include "common/effects.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void effect_slab_the_killer::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::apply_bang_modifiers>(card_id, [p](player *target, request_bang &req) {
            if (p == target) {
                ++req.bang_strength;
            }
        });
    }

    void effect_black_jack::on_play(player *target) {
        int ncards = target->m_num_drawn_cards;
        for (int i=0; i<ncards; ++i) {
            if (i==1) {
                auto removed = target->m_game->draw_card();
                if (removed.suit == card_suit_type::hearts || removed.suit == card_suit_type::diamonds) {
                    ++ncards;
                }
                target->m_game->add_show_card(removed, nullptr, true);
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
        auto it = std::ranges::find(target->m_max_cards_mods, 10);
        if (it != target->m_max_cards_mods.end()) {
            target->m_max_cards_mods.erase(it);
        }
    }

    void effect_kit_carlson::on_play(player *target) {
        for (int i=0; i<=target->m_num_drawn_cards; ++i) {
            target->m_game->add_to_temps(target->m_game->draw_card(), target);
        }
        target->m_game->queue_request<request_type::kit_carlson>(target, target);
    }

    void request_kit_carlson::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            target->add_to_hand(target->m_game->draw_from_temp(card_id));
            if (target->m_game->m_temps.size() == 1) {
                target->m_game->pop_request();
                auto removed = std::move(target->m_game->m_temps.front());
                target->m_game->add_private_update<game_update_type::hide_card>(target, removed.id);
                target->m_game->add_public_update<game_update_type::move_card>(removed.id, 0, card_pile_type::main_deck);
                target->m_game->m_temps.clear();
                target->m_game->m_deck.push_back(std::move(removed));
            } else {
                target->m_game->pop_request_noupdate();
                target->m_game->queue_request<request_type::kit_carlson>(target, target);
            }
        }
    }

    void effect_claus_the_saint::on_play(player *target) {
        int ncards = target->m_game->num_alive() + target->m_num_drawn_cards - 1;
        for (int i=0; i<ncards; ++i) {
            target->m_game->add_to_temps(target->m_game->draw_card(), target);
        }
        target->m_game->queue_request<request_type::claus_the_saint>(target, target);
    }

    void request_claus_the_saint::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            int index = target->m_game->num_alive() + target->m_num_drawn_cards - target->m_game->m_temps.size();
            auto p = target;
            for(int i=0; i<index; ++i) {
                p = target->m_game->get_next_player(p);
            }
            p->add_to_hand(target->m_game->draw_from_temp(card_id));
            if (target->m_game->m_temps.size() == target->m_num_drawn_cards) {
                target->m_game->pop_request();
                for (auto &c : target->m_game->m_temps) {
                    target->add_to_hand(std::move(c));
                }
                target->m_game->m_temps.clear();
            } else {
                target->m_game->pop_request_noupdate();
                target->m_game->queue_request<request_type::claus_the_saint>(nullptr, target);
            }
        }
    }

    void effect_el_gringo::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, bool is_bang) {
            if (origin && p == target && !origin->m_hand.empty() && p->m_game->m_playing != p) {
                target->steal_card(origin, origin->random_hand_card().id);
            }
        });
    }

    void effect_suzy_lafayette::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_effect_end>(card_id, [p](player *origin) {
            if (p->m_hand.empty()) {
                p->add_to_hand(p->m_game->draw_card());
            }
        });
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

    void effect_greg_digger::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_player_death>(card_id, [p](player *origin, player *target) {
            p->heal(2);
        });
    }

    void effect_herb_hunter::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_player_death>(card_id, [p](player *origin, player *target) {
            p->add_to_hand(p->m_game->draw_card());
            p->add_to_hand(p->m_game->draw_card());
        });
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

    void effect_molly_stark::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_play_hand_card>(card_id, [p](player *target, int player_card) {
            if (p == target && p->m_game->m_playing != p) {
                p->add_to_hand(p->m_game->draw_card());
            }
        });
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
        target->m_game->remove_events(card_id);
    }

    static void vera_custer_copy_character(player *target, const character &c) {
        if (c.name != target->m_characters.back().name) {
            auto copy = c;
            copy.id = target->m_game->get_next_id();
            copy.usages = 0;

            if (target->m_characters.size() == 2) {
                target->m_characters.back().on_unequip(target);
                target->m_characters.pop_back();
            }
            target->send_character_update(copy, 1);
            target->m_characters.emplace_back(std::move(copy)).on_equip(target);
        }
    }

    void effect_vera_custer::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_turn_start>(card_id, [p](player *target) {
            if (p == target) {
                ++p->m_characters.front().usages;
                if (p->m_game->num_alive() == 2 && p->m_game->get_next_player(p)->m_characters.size() == 1) {
                    vera_custer_copy_character(p, p->m_game->get_next_player(p)->m_characters.front());
                } else if (p->m_game->num_alive() > 2) {
                    p->m_game->queue_request<request_type::vera_custer>(p, p);
                }
            }
        });
        p->m_game->add_event<event_type::on_turn_end>(card_id, [p](player *target) {
            if (p == target && p->m_characters.front().usages == 0) {
                if (p->m_characters.size() > 1) {
                    p->m_characters.back().on_unequip(p);
                    p->m_characters.pop_back();
                    p->m_game->add_public_update<game_update_type::player_character>(p->id, 0, 1);
                }
            }
        });
    }

    void effect_vera_custer::on_unequip(player *target, int card_id) {
        target->m_game->remove_events(card_id);

        if (target->m_characters.size() > 1) {
            target->m_characters.pop_back();
            target->m_game->add_public_update<game_update_type::player_character>(target->id, 0, 1);
        }
    }

    void request_vera_custer::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_character) {
            if (card_id != target->m_characters.front().id) {
                target->m_game->pop_request();
                vera_custer_copy_character(target, target->m_game->find_character(card_id));
            }
        }
    }

    void effect_tuco_franziskaner::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_draw_from_deck>(card_id, [p](player *origin) {
            if (p == origin && std::ranges::none_of(p->m_table, [](const deck_card &c) {
                return c.color == card_color_type::blue;
            })) {
                p->add_to_hand(p->m_game->draw_card());
                p->add_to_hand(p->m_game->draw_card());
            }
        });
    }

    void effect_colorado_bill::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::apply_bang_modifiers>(card_id, [p](player *origin, request_bang &req) {
            if (p == origin) {
                origin->m_game->draw_check_then(origin, [&](card_suit_type suit, card_value_type) {
                    if (suit == card_suit_type::spades) {
                        req.unavoidable = true;
                    }
                });
            }
        });
    }

    void effect_henry_block::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_discard_card>(card_id, [p](player *origin, player *target, int card_id) {
            if (p == target && p != origin) {
                p->m_game->queue_request<request_type::bang>(target, origin);
            }
        });
    }
}