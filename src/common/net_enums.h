#ifndef __NET_ENUMS_H__
#define __NET_ENUMS_H__

#include "game_update.h"

DEFINE_SERIALIZABLE(connect_args,
    (user_name, std::string)
)

DEFINE_SERIALIZABLE(lobby_info,
    (name, std::string)
    (expansions, banggame::card_expansion_type)
)

DEFINE_SERIALIZABLE(lobby_join_args,
    (lobby_id, int)
)

DEFINE_SERIALIZABLE(lobby_chat_client_args,
    (message, std::string)
)

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

DEFINE_SERIALIZABLE(lobby_data,
    (lobby_id, int)
    (name, std::string)
    (num_players, int)
    (state, lobby_state)
)

DEFINE_SERIALIZABLE(lobby_player_data,
    (user_id, int)
    (name, std::string)
)

DEFINE_SERIALIZABLE(lobby_entered_args,
    (info, lobby_info)
    (user_id, int)
    (owner_id, int)
)

DEFINE_SERIALIZABLE(lobby_left_args,
    (user_id, int)
)

DEFINE_SERIALIZABLE(lobby_chat_args,
    (user_id, int)
    (message, std::string)
)

DEFINE_SERIALIZABLE(game_started_args,
    (expansions, banggame::card_expansion_type)
)

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
    (game_error, std::string)
    (game_update, game_update)
)

#endif