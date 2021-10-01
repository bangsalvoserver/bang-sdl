#ifndef __GAME_UPDATE_H__
#define __GAME_UPDATE_H__

#include "card_enums.h"
#include "responses.h"

#include "utils/json_serial.h"

namespace banggame {

    DEFINE_SERIALIZABLE(game_over_update,
        (winner_role, player_role)
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

    DEFINE_SERIALIZABLE(virtual_card_update,
        (card_id, int)
        (virtual_id, int)
        (suit, card_suit_type)
        (value, card_value_type)
        (color, card_color_type)
        (targets, std::vector<card_target_data>)
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
        (max_hp, int)
        (name, std::string)
        (image, std::string)
        (type, character_type)
        (targets, std::vector<card_target_data>)
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
        (game_over, game_over_update)
        (add_cards, add_cards_update)
        (move_card, move_card_update)
        (deck_shuffled)
        (virtual_card, virtual_card_update)
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
}

#endif