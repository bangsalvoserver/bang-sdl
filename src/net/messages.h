#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "game_options.h"

#include "utils/image_pixels.h"
#include "utils/enum_bitset.h"

namespace banggame {
    
    using id_type = unsigned int;

    struct user_info {
        std::string name;
        utils::image_pixels profile_image;
    };

    struct connect_args {
        user_info user;
        id_type session_id;
    };

    struct lobby_info {
        std::string name;
        game_options options;
    };

    struct lobby_id_args {
        id_type lobby_id;
    };

    struct lobby_chat_client_args {
        std::string message;
    };

    struct game_rejoin_args {
        int user_id;
    };

    using client_message = utils::tagged_variant<
        utils::tag<"pong">,
        utils::tag<"connect", connect_args>,
        utils::tag<"user_edit", user_info>,
        utils::tag<"lobby_make", lobby_info>,
        utils::tag<"lobby_edit", lobby_info>,
        utils::tag<"lobby_join", lobby_id_args>,
        utils::tag<"lobby_leave">,
        utils::tag<"lobby_chat", lobby_chat_client_args>,
        utils::tag<"lobby_return">,
        utils::tag<"game_start">,
        utils::tag<"game_rejoin", game_rejoin_args>,
        utils::tag<"game_action", json::json>
    >;

    enum class lobby_state {
        waiting,
        playing,
        finished,
    };

    struct client_accepted_args {
        id_type session_id;
    };

    struct lobby_data {
        id_type lobby_id;
        std::string name;
        int num_players;
        int num_spectators;
        int max_players;
        lobby_state state;
    };

    struct lobby_entered_args {
        int user_id;
        id_type lobby_id;
        std::string name;
        game_options options;
    };

    enum class lobby_chat_flag {
        is_read
    };

    struct user_info_id_args {
        int user_id;
        user_info user;
        enums::bitset<lobby_chat_flag> flags;
        std::chrono::milliseconds lifetime;
    };

    struct lobby_chat_args {
        int user_id;
        std::string username;
        std::string message;
        enums::bitset<lobby_chat_flag> flags;
    };

    using server_message = utils::tagged_variant<
        utils::tag<"ping">,
        utils::tag<"client_accepted", client_accepted_args>,
        utils::tag<"client_count", int>,
        utils::tag<"lobby_error", std::string>,
        utils::tag<"lobby_update", lobby_data>,
        utils::tag<"lobby_entered", lobby_entered_args>,
        utils::tag<"lobby_edited", lobby_info>,
        utils::tag<"lobby_removed", lobby_id_args>,
        utils::tag<"lobby_add_user", user_info_id_args>,
        utils::tag<"lobby_remove_user", int>,
        utils::tag<"lobby_kick">,
        utils::tag<"lobby_chat", lobby_chat_args>,
        utils::tag<"lobby_message", std::string>,
        utils::tag<"game_update", json::json>,
        utils::tag<"game_started">
    >;

    template<utils::fixed_string Name>
    concept server_message_type = utils::tag_for<utils::tag<Name>, server_message>;

    #define TAG(name) utils::tag<#name>

}

#endif