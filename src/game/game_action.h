#ifndef __GAME_ACTION_H__
#define __GAME_ACTION_H__

#include "utils/reflector.h"

#include "card_enums.h"

namespace banggame {

    struct pick_card_args {REFLECTABLE(
        (card_pile_type) pile,
        (int) player_id,
        (int) card_id
    )};

    struct play_card_args {REFLECTABLE(
        (int) card_id,
        (std::vector<int>) modifier_ids,
        (std::vector<play_card_target>) targets
    )};

    DEFINE_ENUM_TYPES_IN_NS(banggame, game_action_type,
        (pick_card, pick_card_args)
        (play_card, play_card_args)
        (respond_card, play_card_args)
    )

    using game_action = enums::enum_variant<game_action_type>;
    
}

#endif