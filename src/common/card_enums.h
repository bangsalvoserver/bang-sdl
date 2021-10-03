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

    DEFINE_ENUM_IN_NS(banggame, character_type,
        (none)
        (drawing)
        (drawing_forced)
        (active)
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
        (self)
        (notself)
        (notsheriff)
        (reachable)
        (everyone)
        (table)
        (hand)
        (blue)
        (clubs)
        (bang)
        (missed)
        (bangormissed)
    )

    DEFINE_ENUM_IN_NS(banggame, card_pile_type,
        (player_hand)
        (player_table)
        (player_character)
        (main_deck)
        (discard_pile)
        (temp_table)
    )

    struct player;

    DEFINE_ENUM_TYPES_IN_NS(banggame, event_type,
        (on_hit,            std::function<void(player *origin, player *target)>)
        (on_player_death,   std::function<void(player *origin, player *target)>)
        (on_equip,          std::function<void(player *origin, int card_id)>)
        (on_play_off_turn,  std::function<void(player *origin, int card_id)>)
        (on_effect_end,     std::function<void(player *origin)>)
        (on_turn_start,     std::function<void(player *origin)>)
        (on_turn_end,       std::function<void(player *origin)>)
    )

    using event_function = enums::enum_variant<event_type>;

}

#endif