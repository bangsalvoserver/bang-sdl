#ifndef __UPDATE_ENUMS_H__
#define __UPDATE_ENUMS_H__

#include "card_enums.h"
#include "responses.h"

#include "utils/json_serial.h"

namespace banggame {

    DEFINE_ENUM_IN_NS(banggame, game_notify_type,
        (game_over)
        (deck_shuffled)
    )

    DEFINE_SERIALIZABLE(add_cards_update,
        (card_ids, std::vector<int>)
    )

    DEFINE_SERIALIZABLE(move_card_update,
        (card_id, int)
        (player_id, int)
        (pile, card_pile_type)
    )

    DEFINE_SERIALIZABLE(card_target_data,
        (target, target_type)
        (maxdistance, int)
    )

    DEFINE_SERIALIZABLE(show_card_update,
        (card_id, int)
        (name, std::string)
        (image, std::string)
        (suit, card_suit_type)
        (value, card_value_type)
        (color, card_color_type)
        (targets, std::vector<card_target_data>)
    )

    DEFINE_SERIALIZABLE(hide_card_update,
        (card_id, int)
    )

    DEFINE_SERIALIZABLE(tap_card_update,
        (card_id, int)
        (inactive, bool)
    )

    DEFINE_SERIALIZABLE(player_id_update,
        (player_id, int)
    )

    DEFINE_SERIALIZABLE(player_hp_update,
        (player_id, int)
        (hp, int)
    )

    DEFINE_SERIALIZABLE(player_character_update,
        (player_id, int)
        (card_id, int)
        (name, std::string)
        (image, std::string)
        (target, target_type)
    )

    DEFINE_SERIALIZABLE(player_show_role_update,
        (player_id, int)
        (role, player_role)
    )

    DEFINE_SERIALIZABLE(switch_turn_update,
        (player_id, int)
    )

    DEFINE_SERIALIZABLE(response_handle_update,
        (type, response_type)
        (origin_id, int)
        (target_id, int)
    )

    DEFINE_ENUM_TYPES_IN_NS(banggame, game_update_type,
        (game_notify, game_notify_type)
        (add_cards, add_cards_update)
        (move_card, move_card_update)
        (show_card, show_card_update)
        (hide_card, hide_card_update)
        (tap_card, tap_card_update)
        (player_add, player_id_update)
        (player_own_id, player_id_update)
        (player_hp, player_hp_update)
        (player_character, player_character_update)
        (player_show_role, player_show_role_update)
        (switch_turn, switch_turn_update)
        (response_handle, response_handle_update)
        (response_done)
    )

    using game_update = enums::enum_variant<game_update_type>;

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
        (card_id, int)
    )

    DEFINE_SERIALIZABLE(play_card_args,
        (card_id, int)
        (targets, std::vector<play_card_target>)
    )

    DEFINE_ENUM_TYPES_IN_NS(banggame, game_action_type,
        (pick_card, pick_card_args)
        (play_card, play_card_args)
        (respond_card, play_card_args)
        (pass_turn)
        (resolve)
    )

    using game_action = enums::enum_variant<game_action_type>;
}

#endif