#include "effects.h"

#include "../player.h"
#include "../game.h"

namespace banggame {
    void effect_bang::on_resolve(player *target) {
        target->damage(1);
    }

    bool effect_bangcard::on_respond(card_effect *effect, player_card *origin, player *target) {
        if (dynamic_cast<effect_duel *>(effect)) {
            target->play_card(origin->card, origin->player);
            return true;
        } else if (dynamic_cast<effect_indians *>(effect)) {
            return true;
        } else {
            return false;
        }
    }

    bool effect_barrel::on_respond(card_effect *effect, player_card *, player *target) {
        if (dynamic_cast<effect_bang *>(effect)) {
            return target->get_game()->draw_check()->suit == card_suit_type::hearts;
        }
        return false;
    }

    bool effect_heal::on_play(player *, player *target) {
        target->heal(1);
        return true;
    }

    bool effect_beer::on_play(player *, player *target) {
        if (target->get_game()->num_players() > 2) {
            target->heal(1);
        }
        return true;
    }

    bool effect_missed::on_respond(card_effect *effect, player_card *, player *) {
        return dynamic_cast<effect_bang*>(effect) != nullptr;
    }

    bool effect_destroy::on_play(player *, player_card *target) {
        target->player->discard_card(target->card);
        return true;
    }

    bool effect_steal::on_play(player *origin, player_card *target) {
        origin->steal_card(target->player, target->card);
        return true;
    }

    void effect_mustang::on_equip(player *target) {
        target->add_distance(1);
    }

    void effect_mustang::on_unequip(player *target) {
        target->add_distance(-1);
    }

    void effect_scope::on_equip(player *target) {
        target->add_range(1);
    }

    void effect_scope::on_unequip(player *target) {
        target->add_range(-1);
    }

    void effect_jail::on_equip(player *target) {
        target->add_predraw_check(this, 1);
    }

    void effect_jail::on_unequip(player *target) {
        target->remove_predraw_check(this);
    }

    bool effect_jail::on_predraw_check(player_card *target) {
        card *check = target->player->get_game()->draw_check();
        target->player->discard_card(target->card);
        return check->suit == card_suit_type::hearts;
    }

    void effect_dynamite::on_equip(player *target) {
        target->add_predraw_check(this, 2);
    }

    void effect_dynamite::on_unequip(player *target) {
        target->remove_predraw_check(this);
    }

    bool effect_dynamite::on_predraw_check(player_card *target) {
        card *check = target->player->get_game()->draw_check();
        if (check->suit == card_suit_type::spades) {
            if (enums::indexof(check->value) >= enums::indexof(card_value_type::value_2) && enums::indexof(check->value) <= enums::indexof(card_value_type::value_9)) {
                target->player->damage(3);
                target->player->discard_card(target->card);
                return true;
            }
        }
        target->player->get_next_player().equip_card(target->player->get_card_removed(target->card));
        return true;
    }

    void effect_weapon::on_equip(player *target) {
        target->discard_weapon();
        target->set_weapon_range(card_effect::maxdistance);
    }

    void effect_weapon::on_unequip(player *target) {
        target->set_weapon_range(1);
    }

    void effect_volcanic::on_equip(player *target) {
        target->add_infinite_bangs(1);
    }

    void effect_volcanic::on_unequip(player *target) {
        target->add_infinite_bangs(-1);
    }

    bool effect_draw::on_play(player *, player *target) {
        target->draw_card();
        return true;
    }
}