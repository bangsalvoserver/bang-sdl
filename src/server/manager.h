#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <list>
#include <map>

#include <json/json.h>

#include "utils/utils.h"

#include "common/options.h"

#include "common/net_enums.h"

#include "game/game.h"

struct user : util::id_counter<user> {
    std::string name;
    banggame::player *controlling = nullptr;
};

class game_manager;

struct lobby : util::id_counter<lobby> {
    std::map<sdlnet::ip_address, user> users;
    sdlnet::ip_address owner;
    std::string name;
    lobby_state state;
    int maxplayers;

    banggame::game game;
    void start_game();
    void send_updates(game_manager &mgr);
};

struct server_message {
    sdlnet::ip_address addr;
    server_message_type type;
    Json::Value value;
};

template<server_message_type E, typename ... Ts>
Json::Value make_message(Ts && ... args) {
    if constexpr (enums::has_type<E>) {
        return json::serialize(enums::enum_type_t<E>{std::forward<Ts>(args) ... });
    } else {
        return Json::nullValue;
    }
}
class game_manager {
public:
    void parse_message(const sdlnet::ip_address &addr, const std::string &str);
    int pending_messages();
    server_message pop_message();

    void client_disconnected(const sdlnet::ip_address &addr);

    template<server_message_type E, typename ... Ts>
    void send_message(const sdlnet::ip_address &addr, Ts && ... args) {
        m_out_queue.emplace_back(addr, E, make_message<E>(std::forward<Ts>(args) ... ));
    }

    template<server_message_type E, typename ... Ts>
    void broadcast_message(const lobby &lobby, Ts && ... args) {
        auto msg = make_message<E>(std::forward<Ts>(args) ... );
        for (const auto &ip : lobby.users | std::views::keys) {
            m_out_queue.emplace_back(ip, E, msg);
        }
    }

private:
    std::list<lobby>::iterator find_lobby(const sdlnet::ip_address &addr);

    void handle_message(enums::enum_constant<client_message_type::lobby_list>, const sdlnet::ip_address &addr);
    void handle_message(enums::enum_constant<client_message_type::lobby_make>, const sdlnet::ip_address &addr, const lobby_make_args &value);
    void handle_message(enums::enum_constant<client_message_type::lobby_join>, const sdlnet::ip_address &addr, const lobby_join_args &value);
    void handle_message(enums::enum_constant<client_message_type::lobby_players>, const sdlnet::ip_address &addr);
    void handle_message(enums::enum_constant<client_message_type::lobby_leave>, const sdlnet::ip_address &addr);
    void handle_message(enums::enum_constant<client_message_type::lobby_chat>, const sdlnet::ip_address &addr, const lobby_chat_client_args &value);
    void handle_message(enums::enum_constant<client_message_type::game_start>, const sdlnet::ip_address &addr);
    void handle_message(enums::enum_constant<client_message_type::game_action>, const sdlnet::ip_address &addr, const banggame::game_action &value);

    std::list<lobby> m_lobbies;
    std::list<server_message> m_out_queue;
};

#endif