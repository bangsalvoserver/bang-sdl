#ifndef __CARD_ENUMS_H__
#define __CARD_ENUMS_H__

#include "common/utils.h"

namespace banggame {

    DEFINE_ENUM_DATA_IN_NS(banggame, card_suit_type,
        (none,      "-")
        (hearts,    "C")
        (diamonds,  "Q")
        (clubs,     "F")
        (spades,    "P")
    )

    DEFINE_ENUM_DATA_IN_NS(banggame, card_value_type,
        (none,      "-")
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
    
    DEFINE_ENUM_FLAGS_DATA_IN_NS(banggame, card_expansion_type,
        (base)
        (thebullet)
        (dodgecity)
        (valleyofshadows)
        (wildwestshow)
        (goldrush)
        (armedanddangerous)
        (highnoon)
        (fistfulofcards)
        (ghostcards)
    )

    DEFINE_ENUM_IN_NS(banggame, card_color_type,
        (none)
        (brown)
        (blue)
        (green)
        (black)
        (orange)
    )

    DEFINE_ENUM_IN_NS(banggame, player_role,
        (unknown)
        (sheriff)
        (deputy)
        (outlaw)
        (renegade)
    )
    
    DEFINE_ENUM_FLAGS_IN_NS(banggame, target_type,
        (card)
        (player)
        (dead)
        (self)
        (notself)
        (notsheriff)
        (maxdistance)
        (reachable)
        (everyone)
        (table)
        (hand)
        (blue)
        (clubs)
        (bang)
        (missed)
        (bangormissed)
        (attacker)
        (fanning_target)
        (new_target)
        (cube_slot)
        (can_repeat)
    )

    DEFINE_ENUM_FLAGS_IN_NS(banggame, effect_flags,
        (escapable)
        (single_target)
    )

    DEFINE_ENUM_IN_NS(banggame, card_pile_type,
        (none)
        (player)
        (player_hand)
        (player_table)
        (player_character)
        (main_deck)
        (discard_pile)
        (selection)
        (shop_deck)
        (shop_discard)
        (shop_selection)
        (hidden_deck)
        (scenario_deck)
        (scenario_card)
    )

    DEFINE_ENUM_IN_NS(banggame, card_modifier_type,
        (none)
        (bangcard)
        (discount)
    )

}

#endif