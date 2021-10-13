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
        int ncards = target->m_num_drawn_cards = target->m_num_cards_to_draw;
        for (int i=0; i<ncards; ++i) {
            if (i==1) {
                auto removed = target->m_game->draw_card();
                if (removed.suit == card_suit_type::hearts || removed.suit == card_suit_type::diamonds) {
                    ++ncards;
                }
                target->m_game->send_card_update(removed, nullptr, true);
                target->add_to_hand(std::move(removed));
            } else {
                target->add_to_hand(target->m_game->draw_card());
            }
        }
    }

    void effect_bill_noface::on_play(player *target) {
        target->m_num_drawn_cards = target->m_num_cards_to_draw;
        int ncards = target->m_num_cards_to_draw - 1 + target->m_max_hp - target->m_hp;
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
        for (int i=0; i<=target->m_num_cards_to_draw; ++i) {
            target->m_game->add_to_temps(target->m_game->draw_card(), target);
        }
        target->m_game->queue_request<request_type::kit_carlson>(target, target);
    }

    void request_kit_carlson::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::temp_table) {
            target->add_to_hand(target->m_game->draw_from_temp(args.card_id));
            if (target->m_game->m_temps.size() == 1) {
                target->m_game->pop_request();
                auto removed = std::move(target->m_game->m_temps.front());
                target->m_game->add_private_update<game_update_type::hide_card>(target, removed.id);
                target->m_game->add_public_update<game_update_type::move_card>(removed.id, 0, card_pile_type::main_deck);
                target->m_game->m_temps.clear();
                target->m_game->m_deck.push_back(std::move(removed));
                target->m_num_drawn_cards = target->m_num_cards_to_draw;
            } else {
                target->m_game->pop_request_noupdate();
                target->m_game->queue_request<request_type::kit_carlson>(target, target);
            }
        }
    }

    void effect_claus_the_saint::on_play(player *target) {
        target->m_num_drawn_cards = target->m_num_cards_to_draw;
        int ncards = target->m_game->num_alive() + target->m_num_cards_to_draw - 1;
        for (int i=0; i<ncards; ++i) {
            target->m_game->add_to_temps(target->m_game->draw_card(), target);
        }
        target->m_game->queue_request<request_type::claus_the_saint>(target, target);
    }

    void request_claus_the_saint::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::temp_table) {
            target->m_num_drawn_cards = target->m_num_cards_to_draw;
            int index = target->m_game->num_alive() + target->m_num_cards_to_draw - target->m_game->m_temps.size();
            auto p = target;
            for(int i=0; i<index; ++i) {
                p = target->m_game->get_next_player(p);
            }
            p->add_to_hand(target->m_game->draw_from_temp(args.card_id));
            if (target->m_game->m_temps.size() == target->m_num_cards_to_draw) {
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
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, int damage, bool is_bang) {
            if (origin && p == target && !origin->m_hand.empty() && p->m_game->m_playing != p) {
                while(damage--) {
                    target->steal_card(origin, origin->random_hand_card().id);
                }
                target->m_game->queue_event<event_type::on_effect_end>(p);
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
            if (p != target) {
                for (auto &c : target->m_table) {
                    if (c.inactive) {
                        c.inactive = false;
                        p->m_game->add_public_update<game_update_type::tap_card>(c.id, false);
                    }
                    p->add_to_hand(std::move(c));
                }
                target->m_table.clear();
                for (auto &c : target->m_hand) {
                    p->add_to_hand(std::move(c));
                }
                target->m_hand.clear();
            }
        });
    }

    void effect_greg_digger::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_player_death>(card_id, [p](player *origin, player *target) {
            if (p != target) {
                p->heal(2);
            }
        });
    }

    void effect_herb_hunter::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_player_death>(card_id, [p](player *origin, player *target) {
            if (p != target) {
                p->add_to_hand(p->m_game->draw_card());
                p->add_to_hand(p->m_game->draw_card());
            }
        });
    }

    void effect_johnny_kisch::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_equip>(card_id, [p](player *origin, player *target, int equipped_card) {
            if (p == origin) {
                const auto &name = target->find_card(equipped_card).name;
                for (auto &other : p->m_game->m_players) {
                    if (other.id == target->id) continue;
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
        p->m_game->add_event<event_type::on_equip>(card_id, [p](player *origin, player *target, int card_id) {
            if (origin == p && target != p) {
                target->find_card(card_id).on_unequip(target);
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
            target->m_game->send_character_update(copy, target->id, 1);
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
                    p->m_game->add_public_update<game_update_type::player_remove_character>(p->id, 1);
                }
            }
        });
    }

    void effect_vera_custer::on_unequip(player *target, int card_id) {
        target->m_game->remove_events(card_id);

        if (target->m_characters.size() > 1) {
            target->m_characters.pop_back();
            target->m_game->add_public_update<game_update_type::player_remove_character>(target->id, 1);
        }
    }

    void request_vera_custer::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::player_character) {
            if (args.card_id != target->m_characters.front().id) {
                target->m_game->pop_request();
                vera_custer_copy_character(target, target->m_game->get_player(args.player_id)->find_character(args.card_id));
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

    void effect_big_spencer::on_equip(player *p, int card_id) {
        p->m_cant_play_missedcard = true;
        p->m_initial_cards = 5;
    }

    void effect_big_spencer::on_unequip(player *p, int card_id) {
        p->m_cant_play_missedcard = false;
        p->m_initial_cards = 0;
    }

    void effect_gary_looter::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_discard_pass>(card_id, [p](player *origin, int card_id) {
            if (p != origin) {
                auto it = std::ranges::find(p->m_game->m_discards | std::views::reverse, card_id, &card::id);
                if (it != p->m_game->m_discards.rend()) {
                    p->add_to_hand(std::move(*it));
                    p->m_game->m_discards.erase(it.base());
                }
            }
        });
    }

    void effect_john_pain::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_draw_check>(card_id, [p](int card_id) {
            if (p->m_hand.size() < 6) {
                auto it = std::ranges::find(p->m_game->m_discards | std::views::reverse, card_id, &card::id);
                if (it != p->m_game->m_discards.rend()) {
                    p->add_to_hand(std::move(*it));
                    p->m_game->m_discards.erase(it.base());
                }
            }
        });
    }

    bool effect_teren_kill::can_respond(player *origin) const {
        return origin->m_game->top_request_is(request_type::death, origin);
    }

    void effect_teren_kill::on_play(player *origin) {
        auto &req = origin->m_game->top_request().get<request_type::death>();
        if (std::ranges::find(req.draw_attempts, origin->id) == req.draw_attempts.end()) {
            req.draw_attempts.push_back(origin->id);
            origin->m_game->draw_check_then(origin, [origin](card_suit_type suit, card_value_type) {
                if (suit != card_suit_type::spades) {
                    origin->m_game->pop_request();
                    origin->m_hp = 1;
                    origin->m_game->add_public_update<game_update_type::player_hp>(origin->id, 1);
                    origin->add_to_hand(origin->m_game->draw_card());
                }
            });
        }
    }

    void effect_youl_grinner::on_equip(player *target, int card_id) {
        target->m_game->add_event<event_type::on_turn_start>(card_id, [target](player *origin) {
            if (target == origin) {
                for (auto &p : target->m_game->m_players) {
                    if (p.alive() && p.id != target->id && p.m_hand.size() > target->m_hand.size()) {
                        target->m_game->queue_request<request_type::youl_grinner>(target, &p);
                    }
                }
            }
        });
    }

    void request_youl_grinner::on_pick(const pick_card_args &args) {
        if (args.pile == card_pile_type::player_hand && args.player_id == target->id) {
            target->m_game->pop_request();
            origin->steal_card(target, args.card_id);
        }
    }

    void effect_flint_westwood::on_play(player *origin, player *target, int card_id) {
        int num_cards = 2;
        for (int i=0; !target->m_hand.empty() && i<2; ++i) {
            origin->steal_card(target, target->random_hand_card().id);
        }
        target->steal_card(origin, origin->m_virtual->first);
    }

    bool effect_lee_van_kliff::can_play(player *origin) const {
        if (origin->m_last_played_card) {
            auto card_it = std::ranges::find(origin->m_game->m_discards | std::views::reverse, origin->m_last_played_card, &deck_card::id);
            return card_it != origin->m_game->m_discards.rend() && card_it->color == card_color_type::brown;
        }
        return false;
    }

    void effect_lee_van_kliff::on_play(player *origin, player *target, int card_id) {
        auto copy = *std::ranges::find(origin->m_game->m_discards | std::views::reverse, origin->m_last_played_card, &deck_card::id);
        copy.id = card_id;
        copy.suit = card_suit_type::none;
        copy.value = card_value_type::none;
        std::erase_if(copy.effects, [](const effect_holder &e) {
            return e.is(effect_type::banglimit);
        });
        origin->play_virtual_card(std::move(copy));
    }

    void effect_don_bell::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_turn_end>(card_id, [=](player *target) {
            if (p == target) {
                int &usages = p->find_character(card_id).max_usages;
                if (usages == 0) {
                    p->m_game->draw_check_then(p, [&](card_suit_type suit, card_value_type) {
                        if (suit == card_suit_type::diamonds || suit == card_suit_type::hearts) {
                            ++usages;
                            p->m_game->m_playing = p;
                        }
                    });
                } else {
                    usages = 0;
                }
            }
        });
    }

    void effect_madam_yto::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_play_beer>(card_id, [p](player *target) {
            p->add_to_hand(p->m_game->draw_card());
        });
    }

    static void greygory_deck_set_characters(player *target) {
        std::ranges::shuffle(target->m_game->m_base_characters, target->m_game->rng);
        for (int i=0; i<2; ++i) {
            auto &c = target->m_characters.emplace_back(target->m_game->m_base_characters[i]);
            c.on_equip(target);
            target->m_game->send_character_update(c, target->id, i+1);
        }
    }

    void effect_greygory_deck::on_play(player *origin, player *target, int card_id) {
        int &usages = target->find_character(card_id).usages;
        if (usages == 0) {
            ++usages;

            for (int i=1; i<target->m_characters.size(); ++i) {
                target->m_characters[i].on_unequip(target);
            }
            target->m_characters.resize(1);
            greygory_deck_set_characters(target);
        }
    }

    void effect_greygory_deck::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_game_start>(card_id, [p] {
            greygory_deck_set_characters(p);
        });
    }

    bool effect_lemonade_jim::can_respond(player *target) const {
        if (target->m_game->top_request_is(request_type::beer)) {
            const auto &vec = target->m_game->top_request().get<request_type::beer>().players;
            return std::ranges::find(vec, target->id) == vec.end();
        }
        return false;
    }

    void effect_lemonade_jim::on_play(player *origin, player *target) {
        target->heal(1);
        target->m_game->top_request().get<request_type::beer>().players.push_back(target->id);
    }

    void effect_lemonade_jim::on_equip(player *origin, int card_id) {
        origin->m_game->add_event<event_type::on_play_beer>(card_id, [origin](player *target) {
            if (origin != target) {
                target->m_game->queue_request<request_type::beer>(nullptr, target).players.push_back(target->id);
            }
        });
    }
}