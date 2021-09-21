#ifndef __CARD_EFFECT_H__
#define __CARD_EFFECT_H__

#include "common/card_enums.h"

#include <concepts>
#include <vector>

namespace banggame {
    struct card_target {
        target_type target = target_type::none;
        int maxdistance = 0;
    };

    struct card {
        int id;
        card_expansion_type expansion;
        card_suit_type suit;
        card_value_type value;
        card_color_type color;
        std::vector<card_target> targets;
        std::string name;
        std::string image;
        bool known;
    };
}

#endif