#ifndef __EQUIPS_CANYONDIABLO_H__
#define __EQUIPS_CANYONDIABLO_H__

#include "card_effect.h"

namespace banggame {

    struct effect_packmule : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_indianguide : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_taxman : predraw_check_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_lastwill : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

}

#endif