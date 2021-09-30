#include "common/characters.h"

#include "player.h"

namespace banggame {
    void effect_slab_the_killer::on_equip(player *target, int card_id) {
        ++target->m_bang_strength;
    }

    void effect_slab_the_killer::on_unequip(player *target, int card_id) {
        --target->m_bang_strength;
    }
}