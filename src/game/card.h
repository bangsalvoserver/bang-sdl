#ifndef __CARD_H__
#define __CARD_H__

#include <vector>
#include <string>

#include "holders.h"
#include "card_enums.h"

namespace banggame {

    struct card_data {REFLECTABLE(
        (int) id,

        (std::string) name,
        (std::string) image,

        (std::vector<effect_holder>) effects,
        (std::vector<effect_holder>) responses,
        (std::vector<effect_holder>) optionals,
        (std::vector<equip_holder>) equips,

        (card_expansion_type) expansion,

        (card_modifier_type) modifier,
        (mth_type) multi_target_handler,
        
        (card_suit_type) suit,
        (card_value_type) value,
        (card_color_type) color
    )

        bool self_equippable() const {
            return equips.empty() || equips.front().target == play_card_target_type::none;
        }
    
        short buy_cost() const {
            return !equips.empty() && equips.back().is(equip_type::buy_cost) ? equips.back().args : 0;
        }
    };

}

#endif