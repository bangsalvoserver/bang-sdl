#ifndef __GOLDRUSH_EQUIPS_H__
#define __GOLDRUSH_EQUIPS_H__

#include "../card_effect.h"

namespace banggame {
    
    struct effect_luckycharm : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_pickaxe {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_calumet : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_gunbelt : event_based_effect {
        int ncards;
        effect_gunbelt(int value) : ncards(value) {}
        
        void on_enable(card *target_card, player *target);
    };

    struct effect_wanted : event_based_effect, effect_prompt_on_self_equip {
        void on_enable(card *target_card, player *target);
    };
}

#endif