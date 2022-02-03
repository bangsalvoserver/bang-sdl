#ifndef __EQUIPS_VALLEYOFSHADOWS_H__
#define __EQUIPS_VALLEYOFSHADOWS_H__

#include "card_effect.h"

namespace banggame {

    struct effect_snake : predraw_check_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_ghost : card_effect {
        void on_pre_equip(player *target, card *target_card);
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_shotgun : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_bounty : event_based_effect {
        void on_equip(player *target, card *target_card);
    };
}

#endif