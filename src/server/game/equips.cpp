#include "common/equips.h"
#include "common/requests.h"

#include "player.h"
#include "game.h"

namespace banggame {
    
    void event_based_effect::on_unequip(player *target, int card_id) {
        target->m_game->remove_events(card_id);
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
        target->add_predraw_check(card_id, 1);
    }

    void effect_jail::on_unequip(player *target, int card_id) {
        target->remove_predraw_check(card_id);
    }

    void effect_jail::on_predraw_check(player *target, int card_id) {
        target->m_game->draw_check_then(target, [=](card_suit_type suit, card_value_type) {
            auto &moved = target->discard_card(card_id);
            if (suit == card_suit_type::hearts) {
                target->next_predraw_check(card_id);
            } else {
                target->end_of_turn();
            }
        });
    }

    void effect_dynamite::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 2);
    }

    void effect_dynamite::on_unequip(player *target, int card_id) {
        target->remove_predraw_check(card_id);
    }

    void effect_dynamite::on_predraw_check(player *target, int card_id) {
        target->m_game->draw_check_then(target, [=](card_suit_type suit, card_value_type value) {
            if (suit == card_suit_type::spades
                && enums::indexof(value) >= enums::indexof(card_value_type::value_2)
                && enums::indexof(value) <= enums::indexof(card_value_type::value_9)) {
                target->discard_card(card_id);
                target->damage(nullptr, 3);
            } else {
                auto it = std::ranges::find(target->m_table, card_id, &card::id);
                it->on_unequip(target);
                auto moved = std::move(*it);
                target->m_table.erase(it);

                auto *p = target;
                do {
                    p = p->m_game->get_next_player(p);
                } while (p->has_card_equipped(moved.name));
                
                p->equip_card(std::move(moved));
            }
            target->next_predraw_check(card_id);
        });
    }

    void effect_snake::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 0);
    }

    void effect_snake::on_unequip(player *target, int card_id) {
        target->remove_predraw_check(card_id);
    }

    void effect_snake::on_predraw_check(player *target, int card_id) {
        target->m_game->draw_check_then(target, [=](card_suit_type suit, card_value_type value) {
            if (suit == card_suit_type::spades) {
                target->damage(nullptr, 1);
            }
            target->next_predraw_check(card_id);
        });
    }

    void effect_weapon::on_equip(player *target, int card_id) {
        target->discard_weapon(card_id);
        target->m_weapon_range = maxdistance;
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
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, bool is_bang){
            if (p == target) {
                target->add_to_hand(target->m_game->draw_card());
            }
        });
    }

    void effect_horsecharm::on_equip(player *target, int card_id) {
        ++target->m_num_checks;
    }

    void effect_horsecharm::on_unequip(player *target, int card_id) {
        --target->m_num_checks;
    }

    void effect_pickaxe::on_equip(player *target, int card_id) {
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
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, bool is_bang) {
            if (origin == p && target != p && !target->m_hand.empty() && is_bang) {
                target->m_game->queue_request<request_type::discard>(origin, target);
            }
        });
    }

    void effect_bounty::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, bool is_bang) {
            if (origin && target == p && is_bang) {
                origin->add_to_hand(origin->m_game->draw_card());
            }
        });
    }
}