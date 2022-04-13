#ifndef __EQUIPS_BASE_H__
#define __EQUIPS_BASE_H__

#include "card_effect.h"

namespace banggame {

    struct effect_max_hp {
        int value;
        effect_max_hp(int value) : value(value) {}

        void on_equip(card *target_card, player *target);
    };

    struct effect_mustang {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_scope {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };

    struct effect_jail : predraw_check_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_dynamite : predraw_check_effect {
        void on_equip(card *target_card, player *target);
    };

    struct effect_horse {
        opt_fmt_str on_prompt(card *target_card, player *target) const;
        void on_pre_equip(card *target_card, player *target);
    };

    struct effect_weapon {
        int range;
        effect_weapon(int value) : range(value) {}
        
        opt_fmt_str on_prompt(card *target_card, player *target) const;
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

    struct effect_horsecharm {
        void on_equip(card *target_card, player *target);
        void on_unequip(card *target_card, player *target);
    };
}

#endif