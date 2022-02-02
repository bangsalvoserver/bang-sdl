#ifndef __EQUIPS_GOLDRUSH_H__
#define __EQUIPS_GOLDRUSH_H__

#include "card_effect.h"

namespace banggame {
    
    struct effect_luckycharm : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_pickaxe : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_calumet : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_gunbelt : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_wanted : event_based_effect {
        void on_equip(player *target, card *target_card);
    };
}

#endif