#include "common/effects/effects_base.h"

#include "../game.h"

namespace banggame {

    
    void event_based_effect::on_unequip(player *target, card *target_card) {
        target->m_game->remove_events(target_card);
    }

    void predraw_check_effect::on_unequip(player *target, card *target_card) {
        target->remove_predraw_check(target_card);
    }

    void effect_mustang::on_equip(player *target, card *target_card) {
        ++target->m_distance_mod;
        target->send_player_status();
    }

    void effect_mustang::on_unequip(player *target, card *target_card) {
        --target->m_distance_mod;
        target->send_player_status();
    }

    void effect_scope::on_equip(player *target, card *target_card) {
        ++target->m_range_mod;
        target->send_player_status();
    }

    void effect_scope::on_unequip(player *target, card *target_card) {
        --target->m_range_mod;
        target->send_player_status();
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

    void effect_horse::on_pre_equip(player *target, card *target_card) {
        const auto is_horse = [=](const card *c) {
            return c != target_card && !c->equips.empty() && c->equips.front().is(equip_type::horse);
        };
        if (auto it = std::ranges::find_if(target->m_table, is_horse); it != target->m_table.end()) {
            target->discard_card(*it);
        }
    }

    void effect_weapon::on_pre_equip(player *target, card *target_card) {
        const auto is_weapon = [](const card *c) {
            return !c->equips.empty() && c->equips.front().is(equip_type::weapon);
        };
        if (auto it = std::ranges::find_if(target->m_table, is_weapon); it != target->m_table.end()) {
            target->discard_card(*it);
        }
    }

    void effect_weapon::on_equip(player *target, card *target_card) {
        target->m_weapon_range = args;
        target->send_player_status();
    }

    void effect_weapon::on_unequip(player *target, card *target_card) {
        target->m_weapon_range = 1;
        target->send_player_status();
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
}