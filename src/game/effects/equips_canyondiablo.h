#ifndef __EQUIPS_CANYONDIABLO_H__
#define __EQUIPS_CANYONDIABLO_H__

#include "card_effect.h"

namespace banggame {

    struct effect_packmule : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_indianguide : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_taxman : predraw_check_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_brothel : predraw_check_effect {
        static inline uint8_t effect_holder_counter = 0;

        void on_equip(card *target_card, player *target);
    };

    struct effect_bronco {
        void on_pre_equip(card *target_card, player *target);
    };

}

#endif