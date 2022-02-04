#ifndef __EQUIPS_VALLEYOFSHADOWS_H__
#define __EQUIPS_VALLEYOFSHADOWS_H__

#include "card_effect.h"

namespace banggame {

    struct effect_snake : predraw_check_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_ghost : card_effect {
        void on_pre_equip(card *target_card, player *target);
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_shotgun : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_bounty : event_based_effect {
        void on_equip(card *target_card, player *target);
    };
}

#endif