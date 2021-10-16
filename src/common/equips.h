#ifndef __EQUIPS_H__
#define __EQUIPS_H__

#include "effects.h"

namespace banggame {

    struct event_based_effect : card_effect {
        void on_unequip(player *target, int card_id);
    };

    struct predraw_check_effect : card_effect {
        void on_unequip(player *target, int card_id);
    };

    struct effect_mustang : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_scope : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_jail : predraw_check_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_dynamite : predraw_check_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_snake : predraw_check_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_weapon : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_volcanic : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_boots : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_horsecharm : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_luckycharm : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_pickaxe : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_calumet : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_ghost : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_shotgun : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_bounty : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_gunbelt : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

}

#endif