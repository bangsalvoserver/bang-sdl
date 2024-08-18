#ifndef __GAME_UPDATE_H__
#define __GAME_UPDATE_H__

#include "game_options.h"
#include "card_data.h"

#include "utils/tagged_variant.h"
#include "utils/remove_defaults.h"

namespace banggame {

    struct card_backface_list {
        card_list cards;

        card_backface_list() = default;
        card_backface_list(card_list cards): cards{std::move(cards)} {}
        card_backface_list(card_ptr card): cards{card} {}
    };

    struct add_cards_update {
        card_backface_list card_ids;
        pocket_type pocket;
        nullable_player player;
    };

    struct remove_cards_update {
        card_list cards;
    };

    struct move_card_update {
        card_ptr card;
        nullable_player player;
        pocket_type pocket;
        animation_duration duration;
        bool front = false;
    };

    struct add_cubes_update {
        int num_cubes;
        nullable_card target_card;
    };

    struct move_cubes_update {
        int num_cubes;
        nullable_card origin_card;
        nullable_card target_card;
        animation_duration duration;
    };

    struct move_train_update {
        int position;
        animation_duration duration;
    };

    struct deck_shuffled_update {
        pocket_type pocket;
        animation_duration duration;
    };

    struct show_card_update {
        card_ptr card;
        card_data info;
        animation_duration duration;
    };

    struct hide_card_update {
        card_ptr card;
        animation_duration duration;
    };

    struct tap_card_update {
        card_ptr card;
        bool inactive;
        animation_duration duration;
    };

    struct flash_card_update {
        card_ptr card;
        animation_duration duration;
    };

    struct short_pause_update {
        nullable_card card;
        animation_duration duration;
    };

    struct player_user_list {
        player_list players;

        player_user_list() = default;
        player_user_list(player_list players): players{std::move(players)} {}
        player_user_list(player_ptr player): players{player} {}
    };

    struct player_add_update {
        player_user_list players;
    };

    struct player_order_update {
        player_list players;
        animation_duration duration;
    };

    struct player_hp_update {
        player_ptr player;
        int hp;
        animation_duration duration;
    };

    struct player_gold_update {
        player_ptr player;
        int gold;
    };

    struct player_show_role_update {
        player_ptr player;
        player_role role;
        animation_duration duration;
    };

    struct player_flags_update {
        player_ptr player;
        player_flags flags;
    };

    struct playable_card_info {
        card_ptr card;
        card_list modifiers;
        utils::remove_defaults<effect_context> context;
    };

    using playable_cards_list = std::vector<playable_card_info>;

    struct player_distance_item {
        player_ptr player;
        int value;
    };

    struct player_distances {
        std::vector<player_distance_item> distance_mods;
        int range_mod;
        int weapon_range;
    };

    using timer_id_t = size_t;

    struct timer_status_args {
        timer_id_t timer_id;
        game_duration duration;
    };

    struct request_status_args {
        nullable_card origin_card;
        nullable_player origin;
        nullable_player target;
        game_string status_text;
        playable_cards_list respond_cards;
        card_list highlight_cards;
        player_list target_set_players;
        card_list target_set_cards;
        player_distances distances;
        std::optional<timer_status_args> timer;
    };

    struct status_ready_args {
        playable_cards_list play_cards;
        player_distances distances;
    };

    using game_update = utils::tagged_variant<
        utils::tag<"game_error", game_string>,
        utils::tag<"game_log", game_string>,
        utils::tag<"game_prompt", game_string>,
        utils::tag<"add_cards", add_cards_update>,
        utils::tag<"remove_cards", remove_cards_update>,
        utils::tag<"move_card", move_card_update>,
        utils::tag<"add_cubes", add_cubes_update>,
        utils::tag<"move_cubes", move_cubes_update>,
        utils::tag<"move_train", move_train_update>,
        utils::tag<"deck_shuffled", deck_shuffled_update>,
        utils::tag<"show_card", show_card_update>,
        utils::tag<"hide_card", hide_card_update>,
        utils::tag<"tap_card", tap_card_update>,
        utils::tag<"flash_card", flash_card_update>,
        utils::tag<"short_pause", short_pause_update>,
        utils::tag<"player_add", player_add_update>,
        utils::tag<"player_order", player_order_update>,
        utils::tag<"player_hp", player_hp_update>,
        utils::tag<"player_gold", player_gold_update>,
        utils::tag<"player_show_role", player_show_role_update>,
        utils::tag<"player_flags", player_flags_update>,
        utils::tag<"switch_turn", player_ptr>,
        utils::tag<"request_status", request_status_args>,
        utils::tag<"status_ready", status_ready_args>,
        utils::tag<"game_flags", game_flags>,
        utils::tag<"play_sound", std::string>,
        utils::tag<"status_clear">,
        utils::tag<"clear_logs">
    >;

    template<utils::fixed_string Name>
    concept game_update_type = utils::tag_for<utils::tag<Name>, game_update>;

    struct modifier_pair {
        card_ptr card;
        target_list targets;
    };

    using modifier_list = std::vector<modifier_pair>;

    struct game_action {
        card_ptr card;
        modifier_list modifiers;
        target_list targets;
        bool bypass_prompt;
        std::optional<timer_id_t> timer_id;
    };
}

#endif