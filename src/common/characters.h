#ifndef __CHARACTERS_H__
#define __CHARACTERS_H__

#include "card_effect.h"

namespace banggame {
    struct effect_slab_the_killer : card_effect {
        virtual void on_equip(player *target_player, card *target_card) override;
        virtual void on_unequip(player *target_player, card *target_card) override;
    };
}

#endif