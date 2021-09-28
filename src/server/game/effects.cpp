#include "common/effects.h"
#include "common/responses.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void effect_bang::on_play(player *origin, player *target) {
        target->get_game()->queue_response<response_type::bang>(origin, target);
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

    void effect_heal::on_play(player *, player *target) {
        target->heal(1);
    }

    void effect_beer::on_play(player *, player *target) {
        if (target->get_game()->num_alive() > 2) {
            target->heal(1);
        }
    }

    void effect_destroy::on_play(player *, player *target_player, card *target_card) {
        target_player->discard_card(target_card);
    }

    void effect_steal::on_play(player *origin, player *target_player, card *target_card) {
        origin->steal_card(target_player, target_card);
    }

    void effect_mustang::on_equip(player *target_player, card *target_card) {
        target_player->add_distance(1);
    }

    void effect_mustang::on_unequip(player *target_player, card *target_card) {
        target_player->add_distance(-1);
    }

    void effect_scope::on_equip(player *target_player, card *target_card) {
        target_player->add_range(1);
    }

    void effect_scope::on_unequip(player *target_player, card *target_card) {
        target_player->add_range(-1);
    }

    void effect_jail::on_equip(player *target_player, card *target_card) {
        target_player->add_predraw_check(target_card->id, 1);
    }

    void effect_jail::on_unequip(player *target_player, card *target_card) {
        target_player->remove_predraw_check(target_card->id);
    }

    void effect_jail::on_predraw_check(player *target_player, card *target_card) {
        target_player->get_game()->draw_check_then(target_player, [=](card_suit_type suit, card_value_type) {
            auto &moved = target_player->discard_card(target_card);
            if (suit == card_suit_type::hearts) {
                target_player->next_predraw_check(moved.id);
            } else {
                target_player->get_game()->next_turn();
            }
        });
    }

    void effect_dynamite::on_equip(player *target_player, card *target_card) {
        target_player->add_predraw_check(target_card->id, 2);
    }

    void effect_dynamite::on_unequip(player *target_player, card *target_card) {
        target_player->remove_predraw_check(target_card->id);
    }

    void effect_dynamite::on_predraw_check(player *target_player, card *target_card) {
        target_player->get_game()->draw_check_then(target_player, [=](card_suit_type suit, card_value_type value) {
            auto card_id = target_card->id;
            if (suit == card_suit_type::spades
                && enums::indexof(value) >= enums::indexof(card_value_type::value_2)
                && enums::indexof(value) <= enums::indexof(card_value_type::value_9)) {
                target_player->discard_card(target_card);
                target_player->damage(3);
            } else {
                auto moved = target_player->get_card_removed(target_card);
                auto *p = target_player;
                do {
                    p = p->get_game()->get_next_player(p);
                } while (p->has_card_equipped(moved.name));
                p->equip_card(std::move(moved));
            }
            target_player->next_predraw_check(card_id);
        });
    }

    void effect_weapon::on_equip(player *target_player, card *target_card) {
        target_player->discard_weapon();
        target_player->set_weapon_range(card_effect::maxdistance);
    }

    void effect_weapon::on_unequip(player *target_player, card *target_card) {
        target_player->set_weapon_range(1);
    }

    void effect_volcanic::on_equip(player *target_player, card *target_card) {
        target_player->add_infinite_bangs(1);
    }

    void effect_volcanic::on_unequip(player *target_player, card *target_card) {
        target_player->add_infinite_bangs(-1);
    }

    void effect_draw::on_play(player *, player *target) {
        target->add_to_hand(target->get_game()->draw_card());
    }

    void effect_horsecharm::on_equip(player *target_player, card *target_card) {
        target_player->add_num_checks(1);
    }

    void effect_horsecharm::on_unequip(player *target_player, card *target_card) {
        target_player->add_num_checks(-1);
    }
}