#ifndef __EQUIPS_BASE_H__
#define __EQUIPS_BASE_H__

#include "card_effect.h"

namespace banggame {

    struct effect_mustang : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_scope : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_jail : predraw_check_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_dynamite : predraw_check_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_horse : card_effect {
        void on_pre_equip(player *target, card *target_card);
    };

    struct effect_weapon : card_effect {
        void on_pre_equip(player *target, card *target_card);
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_volcanic : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_boots : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_horsecharm : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };
}

#endif