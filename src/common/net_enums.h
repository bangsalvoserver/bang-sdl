#ifndef __NET_ENUMS_H__
#define __NET_ENUMS_H__

#include "game_update.h"

struct connect_args {REFLECTABLE(
    (std::string) user_name,
    (std::vector<std::byte>) profile_image
)};

struct lobby_info {REFLECTABLE(
    (std::string) name,
    (banggame::card_expansion_type) expansions
)};

struct lobby_join_args {REFLECTABLE(
    (int) lobby_id
)};

struct lobby_chat_client_args {REFLECTABLE(
    (std::string) message
)};

DEFINE_ENUM_TYPES(client_message_type,
    (connect, connect_args)
    (lobby_list)
    (lobby_make, lobby_info)
    (lobby_edit, lobby_info)
    (lobby_join, lobby_join_args)
    (lobby_players)
    (lobby_leave)
    (lobby_chat, lobby_chat_client_args)
    (game_start)
    (game_action, game_action)
)

DEFINE_ENUM(lobby_state,
    (waiting)
    (playing)
    (finished)
)

struct lobby_data {REFLECTABLE(
    (int) lobby_id,
    (std::string) name,
    (int) num_players,
    (lobby_state) state
)};

struct lobby_player_data {REFLECTABLE(
    (int) user_id,
    (std::string) name,
    (std::vector<std::byte>) profile_image
)};

struct lobby_entered_args {REFLECTABLE(
    (lobby_info) info,
    (int) user_id,
    (int) owner_id
)};

struct lobby_left_args {REFLECTABLE(
    (int) user_id
)};

struct lobby_chat_args {REFLECTABLE(
    (int) user_id,
    (std::string) message
)};

struct game_started_args {REFLECTABLE(
    (banggame::card_expansion_type) expansions
)};

DEFINE_ENUM_TYPES(server_message_type,
    (client_accepted)
    (lobby_list, std::vector<lobby_data>)
    (lobby_update, lobby_data)
    (lobby_entered, lobby_entered_args)
    (lobby_players, std::vector<lobby_player_data>)
    (lobby_edited, lobby_info)
    (lobby_joined, lobby_player_data)
    (lobby_left, lobby_left_args)
    (lobby_chat, lobby_chat_args)
    (game_started, game_started_args)
    (game_update, game_update)
)

#endif