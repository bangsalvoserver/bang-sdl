#ifndef __GAME_ACTION_H__
#define __GAME_ACTION_H__

#include "common/reflector.h"
#include "card_enums.h"

namespace banggame {

    struct target_player_id {REFLECTABLE(
        (int) player_id
    )};

    struct target_card_id {REFLECTABLE(
        (int) player_id,
        (int) card_id
    )};

    DEFINE_ENUM_TYPES_IN_NS(banggame, play_card_target_type,
        (target_none)
        (target_player, std::vector<target_player_id>)
        (target_card, std::vector<target_card_id>)
    )

    using play_card_target = enums::enum_variant<play_card_target_type>;

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
        (draw_from_deck)
        (pass_turn)
        (resolve)
    )

    using game_action = enums::enum_variant<game_action_type>;
    
}

#endif