#include "common/equips.h"
#include "common/requests.h"

#include "player.h"
#include "game.h"

namespace banggame {
    
    void event_based_effect::on_unequip(player *target, int card_id) {
        target->m_game->remove_events(card_id);
    }

    void predraw_check_effect::on_unequip(player *target, int card_id) {
        target->remove_predraw_check(card_id);
    }

    void effect_mustang::on_equip(player *target, int card_id) {
        ++target->m_distance_mod;
    }

    void effect_mustang::on_unequip(player *target, int card_id) {
        --target->m_distance_mod;
    }

    void effect_scope::on_equip(player *target, int card_id) {
        ++target->m_range_mod;
    }

    void effect_scope::on_unequip(player *target, int card_id) {
        --target->m_range_mod;
    }

    void effect_jail::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 1, [=](card_suit_type suit, card_value_type) {
            auto &moved = target->discard_card(card_id);
            if (suit == card_suit_type::hearts) {
                target->next_predraw_check(card_id);
            } else {
                target->m_game->get_next_player(target)->start_of_turn();
            }
        });
    }

    void effect_dynamite::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 2, [=](card_suit_type suit, card_value_type value) {
            if (suit == card_suit_type::spades
                && enums::indexof(value) >= enums::indexof(card_value_type::value_2)
                && enums::indexof(value) <= enums::indexof(card_value_type::value_9)) {
                target->discard_card(card_id);
                target->damage(card_id, nullptr, 3);
            } else {
                auto it = std::ranges::find(target->m_table, card_id, &card::id);

                auto *p = target;
                do {
                    p = p->m_game->get_next_player(p);
                } while (p->has_card_equipped(it->name) && p != target);

                if (p != target) {
                    it->on_unequip(target);
                    p->equip_card(std::move(*it));
                    target->m_table.erase(it);
                }
            }
            target->next_predraw_check(card_id);
        });
    }

    void effect_snake::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 0, [=](card_suit_type suit, card_value_type value) {
            if (suit == card_suit_type::spades) {
                target->damage(card_id, nullptr, 1);
            }
            target->next_predraw_check(card_id);
        });
    }

    void effect_bomb::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 0, [=](card_suit_type suit, card_value_type value) {
            if (suit == card_suit_type::spades || suit == card_suit_type::clubs) {
                target->pay_cubes(target->find_card(card_id), 2);
            } else {
                target->m_game->add_request<request_type::move_bomb>(card_id, nullptr, target);
            }
            target->next_predraw_check(card_id);
        });
        target->m_game->add_event<event_type::post_discard_orange_card>(card_id, [=](player *p, int c_id) {
            if (c_id == card_id && p == target) {
                target->damage(card_id, nullptr, 2);
            }
        });
    }

    void effect_bomb::on_unequip(player *target, int card_id) {
        predraw_check_effect{}.on_unequip(target, card_id);
        event_based_effect{}.on_unequip(target, card_id);
    }

    void effect_weapon::on_equip(player *target, int card_id) {
        auto it = std::ranges::find_if(target->m_table, [card_id](const deck_card &c) {
            return !c.equips.empty() && c.equips.front().is(equip_type::weapon) && c.id != card_id;
        });
        if (it != target->m_table.end()) {
            target->drop_all_cubes(*it);
            target->m_game->move_to(std::move(*it), card_pile_type::discard_pile).on_unequip(target);
            target->m_table.erase(it);
        }
        target->m_weapon_range = args;
    }

    void effect_weapon::on_unequip(player *target, int card_id) {
        target->m_weapon_range = 1;
    }

    void effect_volcanic::on_equip(player *target, int card_id) {
        ++target->m_infinite_bangs;
    }

    void effect_volcanic::on_unequip(player *target, int card_id) {
        --target->m_infinite_bangs;
    }

    void effect_boots::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, int damage, bool is_bang){
            if (p == target) {
                while (damage--) {
                    target->m_game->draw_card_to(card_pile_type::player_hand, target);
                }
            }
        });
    }

    void effect_horsecharm::on_equip(player *target, int card_id) {
        ++target->m_num_checks;
    }

    void effect_horsecharm::on_unequip(player *target, int card_id) {
        --target->m_num_checks;
    }

    void effect_luckycharm::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, int damage, bool is_bang) {
            if (p == target) {
                target->add_gold(damage);
            }
        });
    }

    void effect_pickaxe::on_equip(player *target, int card_id) {
        if (target->m_num_drawn_cards >= target->m_num_cards_to_draw) {
            ++target->m_num_drawn_cards;
        }
        ++target->m_num_cards_to_draw;
    }

    void effect_pickaxe::on_unequip(player *target, int card_id) {
        --target->m_num_cards_to_draw;
    }

    void effect_calumet::on_equip(player *target, int card_id) {
        ++target->m_calumets;
    }

    void effect_calumet::on_unequip(player *target, int card_id) {
        --target->m_calumets;
    }

    void effect_ghost::on_equip(player *target, int card_id) {
        for (auto &c : target->m_characters) {
            c.on_equip(target);
        }

        target->m_ghost = true;
        target->m_game->add_public_update<game_update_type::player_hp>(target->id, 0, false);
        target->m_game->add_event<event_type::post_discard_card>(card_id, [=](player *e_target, int e_card_id) {
            if (card_id == e_card_id && target == e_target) {
                for (auto &c : target->m_characters) {
                    c.on_unequip(target);
                }
                
                target->m_ghost = false;
                target->m_game->add_public_update<game_update_type::player_hp>(target->id, 0, true);
                
                target->m_game->instant_event<event_type::on_player_death>(nullptr, target);
                target->discard_all();

                target->m_game->check_game_over(target, true);
            }
        });
    }

    void effect_shotgun::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [=](player *origin, player *target, int damage, bool is_bang) {
            if (origin == p && target != p && !target->m_hand.empty() && is_bang) {
                target->m_game->queue_request<request_type::discard>(card_id, origin, target);
            }
        });
    }

    void effect_bounty::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, int damage, bool is_bang) {
            if (origin && target == p && is_bang) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
            }
        });
    }

    void effect_gunbelt::on_equip(player *target, int card_id) {
        target->m_max_cards_mods.push_back(args);
    }

    void effect_gunbelt::on_unequip(player *target, int card_id) {
        auto it = std::ranges::find(target->m_max_cards_mods, args);
        if (it != target->m_max_cards_mods.end()) {
            target->m_max_cards_mods.erase(it);
        }
    }

    void effect_wanted::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_player_death>(card_id, [p](player *origin, player *target) {
            if (origin && p == target && origin != target) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                origin->add_gold(1);
            }
        });
    }
}