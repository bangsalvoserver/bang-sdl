#ifndef __EQUIPS_BASE_H__
#define __EQUIPS_BASE_H__

#include "card_effect.h"

namespace banggame {

    struct effect_mustang : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_scope : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_jail : predraw_check_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_dynamite : predraw_check_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_horse : card_effect {
        void on_pre_equip(card *target_card, player *target);
    };

    struct effect_weapon : card_effect {
        void on_pre_equip(card *target_card, player *target);
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_volcanic : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_boots : event_based_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_horsecharm : card_effect {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };
}

#endif