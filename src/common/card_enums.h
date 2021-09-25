#ifndef __CARD_ENUMS_H__
#define __CARD_ENUMS_H__

#include "utils/utils.h"

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
    
    DEFINE_ENUM_FLAGS_IN_NS(banggame, card_expansion_type,
        (base)
        (thebullet)
        (dodgecity)
    )

    DEFINE_ENUM_IN_NS(banggame, card_color_type,
        (brown)
        (blue)
        (green)
    )

    DEFINE_ENUM_IN_NS(banggame, player_role,
        (unknown)
        (sheriff)
        (deputy)
        (outlaw)
        (renegade)
    )
    
    DEFINE_ENUM_IN_NS(banggame, target_type,
        (none)
        (self)
        (notself)
        (everyone)
        (others)
        (notsheriff)
        (reachable)
        (response)
        (selfhand)
        (othercards)
        (anycard)
    )

    DEFINE_ENUM_IN_NS(banggame, card_pile_type,
        (player_hand)
        (player_table)
        (main_deck)
        (discard_pile)
        (temp_table)
    )

}

#endif