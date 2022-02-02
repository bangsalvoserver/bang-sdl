#ifndef __CHARACTERS_VALLEYOFSHADOWS_H__
#define __CHARACTERS_VALLEYOFSHADOWS_H__

#include "card_effect.h"

namespace banggame {
    
    struct effect_tuco_franziskaner : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_colorado_bill : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_henry_block : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_lemonade_jim : event_based_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
        void on_equip(player *target, card *target_card);
    };
}

#endif