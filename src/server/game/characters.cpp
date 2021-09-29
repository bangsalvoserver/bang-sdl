#include "common/characters.h"

#include "player.h"

namespace banggame {
    void effect_slab_the_killer::on_equip(player *target_player, card *target_card) {
        target_player->add_bang_strength(1);
    }

    void effect_slab_the_killer::on_unequip(player *target_player, card *target_card) {
        target_player->add_bang_strength(-1);
    }
}