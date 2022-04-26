#ifndef __CARD_DATA_H__
#define __CARD_DATA_H__

#include <vector>
#include <string>

#include "holders.h"
#include "card_enums.h"

namespace banggame {

    template<typename Holder>
    struct effect_list_base : std::vector<Holder> {
        using enum_type = typename Holder::enum_type;

        bool first_is(enum_type type) const {
            return !this->empty() && this->front().is(type);
        }

        bool last_is(enum_type type) const {
            return !this->empty() && this->back().is(type);
        }
    };

    using effect_list = effect_list_base<effect_holder>;
    using equip_list = effect_list_base<equip_holder>;

    struct card_data {REFLECTABLE(
        (int) id,

        (std::string) name,
        (std::string) image,

        (effect_list) effects,
        (effect_list) responses,
        (effect_list) optionals,
        (equip_list) equips,

        (card_expansion_type) expansion,
        (card_deck_type) deck,

        (card_modifier_type) modifier,
        (mth_holder) multi_target_handler,
        
        (card_sign) sign,
        (card_color_type) color
    )

        bool is_weapon() const {
            return equips.first_is(equip_type::weapon);
        }

        bool self_equippable() const {
            return equips.empty() || equips.front().target == play_card_target_type::none;
        }
    
        short buy_cost() const {
            return equips.last_is(equip_type::buy_cost) ? equips.back().effect_value : 0;
        }
    };

}

#endif