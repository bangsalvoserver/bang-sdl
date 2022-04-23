#ifndef __VALLEYOFSHADOWS_EQUIPS_H__
#define __VALLEYOFSHADOWS_EQUIPS_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_snake : predraw_check_effect, effect_prompt_on_self_equip {
        void on_enable(card *target_card, player *target);
    };

    struct effect_ghost {
        void on_equip(card *target_card, player *target);
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_shotgun : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_bounty : event_based_effect, effect_prompt_on_self_equip {
        void on_enable(card *target_card, player *target);
    };
}

#endif