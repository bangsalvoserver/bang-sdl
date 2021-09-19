#ifndef __CARD_H__
#define __CARD_H__

#include <vector>
#include <string>
#include <memory>

#include "effects/card_effect.h"

namespace banggame {

    DEFINE_ENUM_DATA_IN_NS(banggame, card_suit_type,
        (hearts,    "C")
        (diamonds,  "Q")
        (clubs,     "F")
        (spades,    "P")
    )

    DEFINE_ENUM_DATA_IN_NS(banggame, card_value_type,
        (value_A,   "A")
        (value_2,   "2")
        (value_3,   "3")
        (value_4,   "4")
        (value_5,   "5")
        (value_6,   "6")
        (value_7,   "7")
        (value_8,   "8")
        (value_9,   "9")
        (value_10,  "10")
        (value_J,   "J")
        (value_Q,   "Q")
        (value_K,   "K")
    )

    DEFINE_ENUM_IN_NS(banggame, card_expansion_type,
        (base)
    )

    DEFINE_ENUM_IN_NS(banggame, card_color_type,
        (brown)
        (blue)
    )

    struct effect_list : std::vector<std::unique_ptr<card_effect>> {
        effect_list() = default;
        ~effect_list() = default;

        effect_list(const effect_list &other) {
            *this = other;
        }
        effect_list(effect_list &&other) = default;

        effect_list &operator = (const effect_list &other) {
            for (const auto &obj : other) {
                push_back(std::make_unique<card_effect>(*obj));
            }
            return *this;
        }
        effect_list &operator = (effect_list &&other) = default;
    };

    struct card {
        int id;
        card_expansion_type expansion;
        card_suit_type suit;
        card_value_type value;
        card_color_type color;
        effect_list effects;
        std::string name;
        std::string image;
    };

}

#endif