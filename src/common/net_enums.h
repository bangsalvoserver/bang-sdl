#ifndef __NET_ENUMS_H__
#define __NET_ENUMS_H__

#include "update_enums.h"

DEFINE_SERIALIZABLE(game_error,
    (message, std::string)
)

DEFINE_ENUM(lobby_state,
    (waiting)
    (playing)
    (finished)
)

DEFINE_SERIALIZABLE(lobby_make_args,
    (lobby_name, std::string)
    (player_name, std::string)
    (expansions, banggame::card_expansion_type)
    (max_players, int)
)

DEFINE_SERIALIZABLE(lobby_join_args,
    (lobby_id, int)
    (player_name, std::string)
)

DEFINE_SERIALIZABLE(lobby_chat_client_args,
    (message, std::string)
)

DEFINE_ENUM_TYPES(client_message_type,
    (lobby_list)
    (lobby_make, lobby_make_args)
    (lobby_join, lobby_join_args)
    (lobby_players)
    (lobby_leave)
    (lobby_chat, lobby_chat_client_args)
    (game_start)
    (game_action, game_action)
)

DEFINE_SERIALIZABLE(lobby_data,
    (lobby_id, int)
    (name, std::string)
    (num_players, int)
    (max_players, int)
    (state, lobby_state)
)

DEFINE_SERIALIZABLE(lobby_player_data,
    (user_id, int)
    (name, std::string)
)

DEFINE_SERIALIZABLE(lobby_entered_args,
    (lobby_name, std::string)
    (user_id, int)
    (owner_id, int)
)

DEFINE_SERIALIZABLE(lobby_left_args,
    (user_id, int)
)

DEFINE_SERIALIZABLE(lobby_deleted_args,
    (lobby_id, int)
)

DEFINE_SERIALIZABLE(lobby_chat_args,
    (user_id, int)
    (message, std::string)
)

DEFINE_ENUM_TYPES(server_message_type,
    (game_error, game_error)
    (lobby_list, std::vector<lobby_data>)
    (lobby_entered, lobby_entered_args)
    (lobby_players, std::vector<lobby_player_data>)
    (lobby_joined, lobby_player_data)
    (lobby_left, lobby_left_args)
    (lobby_deleted, lobby_deleted_args)
    (lobby_chat, lobby_chat_args)
    (game_started)
    (game_update, game_update)
)

#endif