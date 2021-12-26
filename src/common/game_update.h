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
        (show_everyone)
    )

    DEFINE_SERIALIZABLE(move_card_update,
        (card_id, int)
        (player_id, int)
        (pile, card_pile_type)
        (flags, show_card_flags)
    )

    DEFINE_SERIALIZABLE(add_cubes_update,
        (cubes, std::vector<int>)
    )

    DEFINE_SERIALIZABLE(move_cube_update,
        (cube_id, int)
        (card_id, int)
    )

    DEFINE_SERIALIZABLE(card_target_data,
        (type, effect_type)
        (target, target_type)
        (args, int)
    )

    DEFINE_SERIALIZABLE(equip_target_data,
        (type, equip_type)
        (target, target_type)
        (args, int)
    )

    DEFINE_SERIALIZABLE(card_info,
        (id, int)
        (expansion, card_expansion_type)
        (name, std::string)
        (image, std::string)
        (targets, std::vector<card_target_data>)
        (response_targets, std::vector<card_target_data>)
        (optional_targets, std::vector<card_target_data>)
        (equip_targets, std::vector<equip_target_data>)
        (modifier, card_modifier_type)
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
        (ignore_player_id, int)
    )

    DEFINE_SERIALIZABLE(tap_card_update,
        (card_id, int)
        (inactive, bool)
        (instant, bool)
    )

    DEFINE_SERIALIZABLE(last_played_card_id,
        (card_id, int)
    )

    DEFINE_SERIALIZABLE(player_user_update,
        (player_id, int)
        (user_id, int)
    )

    DEFINE_SERIALIZABLE(player_hp_update,
        (player_id, int)
        (hp, int)
        (dead, bool)
        (instant, bool)
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

    DEFINE_SERIALIZABLE(request_view,
        (type, request_type)
        (target_id, int)
        (flags, effect_flags)
        (status_text, game_formatted_string)
    )

    DEFINE_ENUM_TYPES_IN_NS(banggame, game_update_type,
        (game_over, game_over_update)
        (game_error, game_formatted_string)
        (game_log, game_formatted_string)
        (add_cards, add_cards_update)
        (move_card, move_card_update)
        (add_cubes, add_cubes_update)
        (move_cube, move_cube_update)
        (deck_shuffled, card_pile_type)
        (show_card, show_card_update)
        (hide_card, hide_card_update)
        (tap_card, tap_card_update)
        (last_played_card, last_played_card_id)
        (player_add, player_user_update)
        (player_hp, player_hp_update)
        (player_gold, player_gold_update)
        (player_add_character, player_character_update)
        (player_remove_character, player_remove_character_update)
        (player_show_role, player_show_role_update)
        (switch_turn, switch_turn_update)
        (request_handle, request_view)
        (status_clear)
    )

    using game_update = enums::enum_variant<game_update_type>;
}

#endif