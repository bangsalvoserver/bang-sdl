#ifndef __GAME_UPDATE_H__
#define __GAME_UPDATE_H__

#include "card_enums.h"
#include "requests.h"
#include "timer.h"

namespace banggame {

    DEFINE_SERIALIZABLE(game_over_update,
        (winner_role, player_role)
    )

    DEFINE_SERIALIZABLE(add_cards_update,
        (card_ids, std::vector<int>)
        (pile, card_pile_type)
    )

    DEFINE_ENUM_FLAGS_IN_NS(banggame, show_card_flags,
        (short_pause)
        (no_animation)
    )

    DEFINE_SERIALIZABLE(move_card_update,
        (card_id, int)
        (player_id, int)
        (pile, card_pile_type)
        (flags, show_card_flags)
    )

    DEFINE_SERIALIZABLE(card_target_data,
        (target, target_type)
        (args, int)
    )

    DEFINE_SERIALIZABLE(virtual_card_update,
        (card_id, int)
        (virtual_id, int)
        (suit, card_suit_type)
        (value, card_value_type)
        (color, card_color_type)
        (targets, std::vector<card_target_data>)
    )

    DEFINE_SERIALIZABLE(card_info,
        (id, int)
        (expansion, card_expansion_type)
        (name, std::string)
        (image, std::string)
        (targets, std::vector<card_target_data>)
        (response_targets, std::vector<card_target_data>)
        (equip_targets, std::vector<card_target_data>)
        (playable_offturn, bool)
    )

    DEFINE_SERIALIZABLE(show_card_update,
        (info, card_info)
        (suit, card_suit_type)
        (value, card_value_type)
        (color, card_color_type)
        (flags, show_card_flags)
    )

    DEFINE_SERIALIZABLE(player_character_update,
        (info, card_info)
        (type, character_type)
        (max_hp, int)
        (player_id, int)
        (index, int)
    )

    DEFINE_SERIALIZABLE(player_remove_character_update,
        (player_id, int)
        (index, int)
    )

    DEFINE_SERIALIZABLE(hide_card_update,
        (card_id, int)
        (flags, show_card_flags)
    )

    DEFINE_SERIALIZABLE(tap_card_update,
        (card_id, int)
        (inactive, bool)
    )

    DEFINE_SERIALIZABLE(player_user_update,
        (player_id, int)
        (user_id, int)
    )

    DEFINE_SERIALIZABLE(player_hp_update,
        (player_id, int)
        (hp, int)
        (dead, bool)
    )

    DEFINE_SERIALIZABLE(player_gold_update,
        (player_id, int)
        (gold, int)
    )

    DEFINE_SERIALIZABLE(player_show_role_update,
        (player_id, int)
        (role, player_role)
        (instant, bool)
    )

    DEFINE_SERIALIZABLE(switch_turn_update,
        (player_id, int)
    )

    DEFINE_SERIALIZABLE(request_handle_update,
        (type, request_type)
        (origin_id, int)
        (target_id, int)
    )

    DEFINE_ENUM_TYPES_IN_NS(banggame, game_update_type,
        (game_over, game_over_update)
        (add_cards, add_cards_update)
        (move_card, move_card_update)
        (deck_shuffled, card_pile_type)
        (virtual_card, virtual_card_update)
        (show_card, show_card_update)
        (hide_card, hide_card_update)
        (tap_card, tap_card_update)
        (player_add, player_user_update)
        (player_hp, player_hp_update)
        (player_gold, player_gold_update)
        (player_add_character, player_character_update)
        (player_remove_character, player_remove_character_update)
        (player_show_role, player_show_role_update)
        (switch_turn, switch_turn_update)
        (request_handle, request_handle_update)
        (status_clear)
    )

    using game_update = enums::enum_variant<game_update_type>;
}

#endif