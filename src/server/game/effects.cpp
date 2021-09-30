#include "common/effects.h"
#include "common/responses.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void effect_bang::on_play(player *origin, player *target) {
        target->get_game()->queue_response<response_type::bang>(origin, target);
    }

    bool effect_bangcard::can_play(player *target) {
        return target->can_play_bang();
    }

    void effect_bangcard::on_play(player *origin, player *target) {
        origin->on_play_bang();
        target->get_game()->queue_response<response_type::bang>(origin, target)->get_data()->bang_strength = origin->get_bang_strength();
    }

    void effect_indians::on_play(player *origin, player *target) {
        target->get_game()->queue_response<response_type::indians>(origin, target);
    }

    void effect_duel::on_play(player *origin, player *target) {
        target->get_game()->queue_response<response_type::duel>(origin, target);
    }

    void effect_generalstore::on_play(player *origin) {
        player *target = origin;
        for (int i=0; i<origin->get_game()->num_alive(); ++i) {
            origin->get_game()->add_to_temps(origin->get_game()->draw_card());

            origin->get_game()->queue_response<response_type::generalstore>(origin, target);
            target = target->get_game()->get_next_player(target);
        }
    }

    void effect_heal::on_play(player *origin, player *target) {
        target->heal(1);
    }

    bool effect_damage::can_play(player *target) {
        return target->get_hp() > 1;
    }

    void effect_damage::on_play(player *origin, player *target) {
        target->damage(origin, 1);
    }

    void effect_beer::on_play(player *origin, player *target) {
        if (target->get_game()->num_alive() > 2) {
            target->heal(1);
        }
    }

    void effect_destroy::on_play(player *origin, player *target, int card_id) {
        target->discard_card(card_id);
    }

    void effect_steal::on_play(player *origin, player *target, int card_id) {
        origin->steal_card(target, card_id);
    }

    void effect_mustang::on_equip(player *target, int card_id) {
        target->add_distance(1);
    }

    void effect_mustang::on_unequip(player *target, int card_id) {
        target->add_distance(-1);
    }

    void effect_scope::on_equip(player *target, int card_id) {
        target->add_range(1);
    }

    void effect_scope::on_unequip(player *target, int card_id) {
        target->add_range(-1);
    }

    void effect_jail::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 1);
    }

    void effect_jail::on_unequip(player *target, int card_id) {
        target->remove_predraw_check(card_id);
    }

    void effect_jail::on_predraw_check(player *target, int card_id) {
        target->get_game()->draw_check_then(target, [=](card_suit_type suit, card_value_type) {
            auto &moved = target->discard_card(card_id);
            if (suit == card_suit_type::hearts) {
                target->next_predraw_check(card_id);
            } else {
                target->get_game()->next_turn();
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
        target->get_game()->draw_check_then(target, [=](card_suit_type suit, card_value_type value) {
            if (suit == card_suit_type::spades
                && enums::indexof(value) >= enums::indexof(card_value_type::value_2)
                && enums::indexof(value) <= enums::indexof(card_value_type::value_9)) {
                target->discard_card(card_id);
                target->damage(nullptr, 3);
            } else {
                auto moved = target->get_card_removed(card_id);
                auto *p = target;
                do {
                    p = p->get_game()->get_next_player(p);
                } while (p->has_card_equipped(moved.name));
                p->equip_card(std::move(moved));
            }
            target->next_predraw_check(card_id);
        });
    }

    void effect_weapon::on_equip(player *target, int card_id) {
        target->discard_weapon();
        target->set_weapon_range(card_effect::maxdistance);
    }

    void effect_weapon::on_unequip(player *target, int card_id) {
        target->set_weapon_range(1);
    }

    void effect_volcanic::on_equip(player *target, int card_id) {
        target->add_infinite_bangs(1);
    }

    void effect_volcanic::on_unequip(player *target, int card_id) {
        target->add_infinite_bangs(-1);
    }

    void effect_draw::on_play(player *origin, player *target) {
        target->add_to_hand(target->get_game()->draw_card());
    }

    bool effect_draw_discard::can_play(player *target) {
        return ! target->get_game()->m_discards.empty();
    }

    void effect_draw_discard::on_play(player *origin, player *target) {
        target->add_to_hand(target->get_game()->draw_from_discards());
    }

    void effect_horsecharm::on_equip(player *target, int card_id) {
        target->add_num_checks(1);
    }

    void effect_horsecharm::on_unequip(player *target, int card_id) {
        target->add_num_checks(-1);
    }
}