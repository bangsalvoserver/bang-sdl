#ifndef __NET_ENUMS_H__
#define __NET_ENUMS_H__

#include "game/game_update.h"

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
    (lobby_leave)
    (lobby_chat, lobby_chat_client_args)
    (lobby_return)
    (game_start)
    (game_action, banggame::game_action)
)

using client_message = enums::enum_variant<client_message_type>;

DEFINE_ENUM(lobby_state,
    (waiting)
    (playing)
    (finished)
)

struct client_accepted_args {REFLECTABLE(
    (int) user_id
)};

struct lobby_data {REFLECTABLE(
    (int) lobby_id,
    (std::string) name,
    (int) num_players,
    (lobby_state) state
)};

struct lobby_add_user_args {REFLECTABLE(
    (int) user_id,
    (std::string) name,
    (std::vector<std::byte>) profile_image
)};

struct lobby_entered_args {REFLECTABLE(
    (lobby_info) info,
    (int) owner_id
)};

struct lobby_remove_user_args {REFLECTABLE(
    (int) user_id
)};

struct lobby_chat_args {REFLECTABLE(
    (int) user_id,
    (std::string) message
)};

DEFINE_ENUM_TYPES(server_message_type,
    (client_accepted, client_accepted_args)
    (lobby_error, std::string)
    (lobby_update, lobby_data)
    (lobby_entered, lobby_entered_args)
    (lobby_edited, lobby_info)
    (lobby_add_user, lobby_add_user_args)
    (lobby_remove_user, lobby_remove_user_args)
    (lobby_chat, lobby_chat_args)
    (game_started, banggame::game_options)
    (game_update, banggame::game_update)
)

using server_message = enums::enum_variant<server_message_type>;

#endif