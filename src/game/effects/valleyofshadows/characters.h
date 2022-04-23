#ifndef __VALLEYOFSHADOWS_CHARACTERS_H__
#define __VALLEYOFSHADOWS_CHARACTERS_H__

#include "../card_effect.h"

namespace banggame {
    
    struct effect_tuco_franziskaner : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_colorado_bill : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_henry_block : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_lemonade_jim : event_based_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
        void on_enable(card *target_card, player *target);
    };
    
    struct effect_evelyn_shebang : event_based_effect {
        opt_error verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };
}

#endif