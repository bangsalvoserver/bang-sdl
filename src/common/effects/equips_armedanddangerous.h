#ifndef __EQUIPS_ARMEDANDDANGEROUS_H__
#define __EQUIPS_ARMEDANDDANGEROUS_H__

#include "card_effect.h"

namespace banggame {
    
    struct effect_bomb : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_tumbleweed : event_based_effect {
        void on_equip(player *target, card *target_card);

        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *target);
    };
}

#endif