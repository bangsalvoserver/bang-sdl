#include "common/equips.h"
#include "common/requests.h"

#include "player.h"
#include "game.h"

namespace banggame {
    
    void event_based_effect::on_unequip(player *target, card *target_card) {
        target->m_game->remove_events(target_card);
    }

    void predraw_check_effect::on_unequip(player *target, card *target_card) {
        target->remove_predraw_check(target_card);
    }

    void effect_mustang::on_equip(player *target, card *target_card) {
        ++target->m_distance_mod;
    }

    void effect_mustang::on_unequip(player *target, card *target_card) {
        --target->m_distance_mod;
    }

    void effect_scope::on_equip(player *target, card *target_card) {
        ++target->m_range_mod;
    }

    void effect_scope::on_unequip(player *target, card *target_card) {
        --target->m_range_mod;
    }

    void effect_jail::on_equip(player *target, card *target_card) {
        target->add_predraw_check(target_card, 1, [=](card *drawn_card) {
            target->discard_card(target_card);
            if (target->get_card_suit(drawn_card) == card_suit_type::hearts) {
                target->next_predraw_check(target_card);
            } else {
                target->m_game->get_next_in_turn(target)->start_of_turn();
            }
        });
    }

    void effect_dynamite::on_equip(player *target, card *target_card) {
        target->add_predraw_check(target_card, 2, [=](card *drawn_card) {
            card_suit_type suit = target->get_card_suit(drawn_card);
            card_value_type value = target->get_card_value(drawn_card);
            if (suit == card_suit_type::spades
                && enums::indexof(value) >= enums::indexof(card_value_type::value_2)
                && enums::indexof(value) <= enums::indexof(card_value_type::value_9)) {
                target->discard_card(target_card);
                target->damage(target_card, nullptr, 3);
            } else {
                auto *p = target;
                do {
                    p = p->m_game->get_next_player(p);
                } while (p->find_equipped_card(target_card) && p != target);

                if (p != target) {
                    target_card->on_unequip(target);
                    p->equip_card(target_card);
                }
            }
            target->next_predraw_check(target_card);
        });
    }

    void effect_snake::on_equip(player *target, card *target_card) {
        target->add_predraw_check(target_card, 0, [=](card *drawn_card) {
            if (target->get_card_suit(drawn_card) == card_suit_type::spades) {
                target->damage(target_card, nullptr, 1);
            }
            target->next_predraw_check(target_card);
        });
    }

    void effect_bomb::on_equip(player *target, card *target_card) {
        target->add_predraw_check(target_card, 0, [=](card *drawn_card) {
            card_suit_type suit = target->get_card_suit(drawn_card);
            if (suit == card_suit_type::spades || suit == card_suit_type::clubs) {
                target->pay_cubes(target_card, 2);
            } else {
                target->m_game->add_request<request_type::move_bomb>(target_card, target);
            }
            target->next_predraw_check(target_card);
        });
        target->m_game->add_event<event_type::post_discard_orange_card>(target_card, [=](player *p, card *c) {
            if (c == target_card && p == target) {
                target->damage(target_card, nullptr, 2);
            }
        });
    }

    void effect_bomb::on_unequip(player *target, card *target_card) {
        predraw_check_effect{}.on_unequip(target, target_card);
        event_based_effect{}.on_unequip(target, target_card);
    }

    void effect_weapon::on_equip(player *target, card *target_card) {
        target->m_weapon_range = args;
    }

    void effect_weapon::on_unequip(player *target, card *target_card) {
        target->m_weapon_range = 1;
    }

    void effect_volcanic::on_equip(player *target, card *target_card) {
        ++target->m_infinite_bangs;
    }

    void effect_volcanic::on_unequip(player *target, card *target_card) {
        --target->m_infinite_bangs;
    }

    void effect_boots::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_hit>(target_card, [p](card *origin_card, player *origin, player *target, int damage, bool is_bang){
            if (p == target) {
                while (damage--) {
                    target->m_game->draw_card_to(card_pile_type::player_hand, target);
                }
            }
        });
    }

    void effect_horsecharm::on_equip(player *target, card *target_card) {
        ++target->m_num_checks;
    }

    void effect_horsecharm::on_unequip(player *target, card *target_card) {
        --target->m_num_checks;
    }

    void effect_luckycharm::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_hit>(target_card, [p](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (p == target) {
                target->add_gold(damage);
            }
        });
    }

    void effect_pickaxe::on_equip(player *target, card *target_card) {
        ++target->m_num_cards_to_draw;
    }

    void effect_pickaxe::on_unequip(player *target, card *target_card) {
        --target->m_num_cards_to_draw;
    }

    void effect_calumet::on_equip(player *target, card *target_card) {
        ++target->m_calumets;
    }

    void effect_calumet::on_unequip(player *target, card *target_card) {
        --target->m_calumets;
    }

    void effect_ghost::on_equip(player *target, card *target_card) {
        if (target_card->pile == card_pile_type::player_hand) {
            if (!target->m_game->characters_disabled(target) && !target->alive()) {
                for (character *c : target->m_characters) {
                    c->on_equip(target);
                }
            }
            target->m_game->add_event<event_type::post_discard_card>(target_card, [=](player *p, card *c) {
                if (p == target && c == target_card) {
                    target->m_ghost = false;
                    target->m_game->player_death(target);
                    target->m_game->check_game_over(target, true);
                    target->m_game->remove_events(target_card);
                }
            });
        }
        target->m_game->add_public_update<game_update_type::player_hp>(target->id, 0, false);
        target->m_ghost = true;
    }

    void effect_ghost::on_unequip(player *target, card *target_card) {
        target->m_game->add_public_update<game_update_type::player_hp>(target->id, 0, true);
        target->m_ghost = false;
    }

    void effect_shotgun::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_hit>(target_card, [=](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin == p && target != p && !target->m_hand.empty() && is_bang) {
                target->m_game->queue_request<request_type::discard>(target_card, origin, target);
            }
        });
    }

    void effect_bounty::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_hit>(target_card, [p](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin && target == p && is_bang) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
            }
        });
    }

    void effect_gunbelt::on_equip(player *target, card *target_card) {
        target->m_max_cards_mods.push_back(args);
    }

    void effect_gunbelt::on_unequip(player *target, card *target_card) {
        auto it = std::ranges::find(target->m_max_cards_mods, args);
        if (it != target->m_max_cards_mods.end()) {
            target->m_max_cards_mods.erase(it);
        }
    }

    void effect_wanted::on_equip(player *p, card *target_card) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p](player *origin, player *target) {
            if (origin && p == target && origin != target) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                origin->add_gold(1);
            }
        });
    }
}