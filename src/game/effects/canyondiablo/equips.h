#ifndef __CANYONDIABLO_EQUIPS_H__
#define __CANYONDIABLO_EQUIPS_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_packmule : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_indianguide : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_taxman : predraw_check_effect, effect_prompt_on_self_equip {
        void on_enable(card *target_card, player *target);
    };

    struct effect_brothel : predraw_check_effect, effect_prompt_on_self_equip {
        static inline uint8_t effect_holder_counter = 0;

        void on_enable(card *target_card, player *target);
    };

    struct effect_bronco {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

}

#endif