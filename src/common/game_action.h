#ifndef __GAME_ACTION_H__
#define __GAME_ACTION_H__

#include "utils/json_serial.h"
#include "card_enums.h"

namespace banggame {

    DEFINE_SERIALIZABLE(target_player_id,
        (player_id, int)
    )
    DEFINE_SERIALIZABLE(target_card_id,
        (player_id, int)
        (card_id, int)
        (from_hand, bool)
    )

    DEFINE_ENUM_TYPES_IN_NS(banggame, play_card_target_type,
        (target_none)
        (target_player, std::vector<target_player_id>)
        (target_card, std::vector<target_card_id>)
    )

    using play_card_target = enums::enum_variant<play_card_target_type>;

    DEFINE_SERIALIZABLE(pick_card_args,
        (pile, card_pile_type)
        (player_id, int)
        (card_id, int)
    )

    DEFINE_ENUM_FLAGS_IN_NS(banggame, play_card_flags,
        (response)
        (equipping)
        (offturn)
        (sell_beer)
        (discard_black)
    )

    DEFINE_SERIALIZABLE(play_card_args,
        (card_id, int)
        (modifier_ids, std::vector<int>)
        (targets, std::vector<play_card_target>)
        (flags, play_card_flags)
    )

    DEFINE_ENUM_TYPES_IN_NS(banggame, game_action_type,
        (pick_card, pick_card_args)
        (play_card, play_card_args)
        (draw_from_deck)
        (pass_turn)
        (resolve)
    )

    using game_action = enums::enum_variant<game_action_type>;
    
}

#endif