#include "common/effects.h"
#include "common/responses.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void effect_bang::on_play(player *origin, player *target) {
        target->m_game->queue_response<response_type::bang>(origin, target);
    }

    void effect_bangcard::on_play(player *origin, player *target) {
        target->m_game->queue_response<response_type::bang>(origin, target)->get_data()->bang_strength = origin->m_bang_strength;
    }

    bool effect_banglimit::can_play(player *target) const {
        return target->can_play_bang();
    }

    void effect_banglimit::on_play(player *origin) {
        ++origin->m_bangs_played;
    }

    void effect_indians::on_play(player *origin, player *target) {
        target->m_game->queue_response<response_type::indians>(origin, target);
    }

    void effect_duel::on_play(player *origin, player *target) {
        target->m_game->queue_response<response_type::duel>(origin, target);
    }

    void effect_generalstore::on_play(player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            origin->m_game->add_to_temps(origin->m_game->draw_card());
        }
        origin->m_game->queue_response<response_type::generalstore>(origin, origin);
    }

    void effect_heal::on_play(player *origin, player *target) {
        target->heal(1);
    }

    bool effect_damage::can_play(player *target) const {
        return target->m_hp > 1;
    }

    void effect_damage::on_play(player *origin, player *target) {
        target->damage(origin, 1);
    }

    void effect_beer::on_play(player *origin, player *target) {
        if (target->m_game->num_alive() > 2) {
            target->heal(target->m_beer_strength);
        }
    }

    void effect_destroy::on_play(player *origin, player *target, int card_id) {
        target->discard_card(card_id);
    }

    void effect_steal::on_play(player *origin, player *target, int card_id) {
        origin->steal_card(target, card_id);
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
                target->m_game->next_turn();
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
                auto moved = target->get_card_removed(card_id);
                auto *p = target;
                do {
                    p = p->m_game->get_next_player(p);
                } while (p->has_card_equipped(moved.name));
                p->equip_card(std::move(moved));
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

    void effect_draw::on_play(player *origin, player *target) {
        target->add_to_hand(target->m_game->draw_card());
    }

    bool effect_draw_discard::can_play(player *target) const {
        return ! target->m_game->m_discards.empty();
    }

    void effect_draw_discard::on_play(player *origin, player *target) {
        target->add_to_hand(target->m_game->draw_from_discards());
    }

    void effect_draw_rest::on_play(player *origin, player *target) {
        for (int i=1; i<target->m_num_drawn_cards; ++i) {
            target->add_to_hand(target->m_game->draw_card());
        }
    }

    void effect_boots::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target){
            if (p == target) {
                target->add_to_hand(target->m_game->draw_card());
            }
        });
    }

    void effect_boots::on_unequip(player *target, int card_id) {
        target->m_game->remove_event(card_id);
    }

    void effect_horsecharm::on_equip(player *target, int card_id) {
        ++target->m_num_checks;
    }

    void effect_horsecharm::on_unequip(player *target, int card_id) {
        --target->m_num_checks;
    }

    void effect_pickaxe::on_equip(player *target, int card_id) {
        ++target->m_num_drawn_cards;
    }

    void effect_pickaxe::on_unequip(player *target, int card_id) {
        --target->m_num_drawn_cards;
    }

    void effect_calumet::on_equip(player *target, int card_id) {
        ++target->m_calumets;
    }

    void effect_calumet::on_unequip(player *target, int card_id) {
        --target->m_calumets;
    }
}