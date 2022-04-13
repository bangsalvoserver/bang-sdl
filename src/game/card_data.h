#ifndef __CARD_DATA_H__
#define __CARD_DATA_H__

#include <vector>
#include <string>

#include "holders.h"
#include "card_enums.h"

namespace banggame {

    struct card_sign {REFLECTABLE(
        (card_suit) suit,
        (card_rank) rank
    )
        explicit operator bool () const {
            return suit != card_suit::none && rank != card_rank::none;
        }
    };

    struct card_data {REFLECTABLE(
        (int) id,

        (std::string) name,
        (std::string) image,

        (std::vector<effect_holder>) effects,
        (std::vector<effect_holder>) responses,
        (std::vector<effect_holder>) optionals,
        (std::vector<equip_holder>) equips,

        (card_expansion_type) expansion,
        (card_deck_type) deck,

        (card_modifier_type) modifier,
        (mth_holder) multi_target_handler,
        
        (card_sign) sign,
        (card_color_type) color
    )

        bool is_weapon() const {
            return !equips.empty() && equips.front().is(equip_type::weapon);
        }

        bool self_equippable() const {
            return equips.empty() || equips.front().target == play_card_target_type::none;
        }
    
        short buy_cost() const {
            return !equips.empty() && equips.back().is(equip_type::buy_cost) ? equips.back().effect_value : 0;
        }
    };

}

#endif