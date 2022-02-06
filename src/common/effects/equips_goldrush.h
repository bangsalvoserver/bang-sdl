#ifndef __EQUIPS_GOLDRUSH_H__
#define __EQUIPS_GOLDRUSH_H__

#include "card_effect.h"

namespace banggame {
    
    struct effect_luckycharm : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_pickaxe : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_calumet : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_gunbelt : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_wanted : event_based_effect {
        void on_equip(card *target_card, player *target);
    };
}

#endif