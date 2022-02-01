#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <list>
#include <map>

#include <json/json.h>

#include "common/utils.h"
#include "common/options.h"
#include "common/net_enums.h"
#include "common/binary_serial.h"

#include "game/game.h"

struct game_user : util::id_counter<game_user> {
    sdlnet::ip_address addr;
    std::string name;
    std::vector<std::byte> profile_image;

    game_user(sdlnet::ip_address addr, std::string name, std::vector<std::byte> profile_image = {})
        : addr(std::move(addr))
        , name(std::move(name))
        , profile_image(std::move(profile_image)) {}
};

class game_manager;

struct lobby_user {
    game_user *user;
    banggame::player *controlling;
};

struct lobby : util::id_counter<lobby> {
    std::vector<lobby_user> users;
    game_user *owner;
    std::string name;
    lobby_state state;
    banggame::card_expansion_type expansions;

    banggame::game game;
    void start_game(const banggame::all_cards_t &all_cards);
    void send_updates(game_manager &mgr);
};

struct server_message_pair {
    sdlnet::ip_address addr;
    server_message value;
};

template<server_message_type E, typename ... Ts>
server_message make_message(Ts && ... args) {
    return server_message{enums::enum_constant<E>{}, std::forward<Ts>(args) ...};
}

#define MESSAGE_TAG(name) enums::enum_constant<client_message_type::name>

using message_callback_t = std::function<void(const std::string &)>;

class game_manager {
public:
    game_manager();
    
    void parse_message(const sdlnet::ip_address &addr, const std::vector<std::byte> &msg);
    int pending_messages();
    server_message_pair pop_message();

    void client_disconnected(const sdlnet::ip_address &addr);

    template<server_message_type E, typename ... Ts>
    void send_message(const sdlnet::ip_address &addr, Ts && ... args) {
        m_out_queue.emplace_back(addr, make_message<E>(std::forward<Ts>(args) ... ));
    }

    template<server_message_type E, typename ... Ts>
    void broadcast_message(const lobby &lobby, Ts && ... args) {
        auto msg = make_message<E>(std::forward<Ts>(args) ... );
        for (const auto &u : lobby.users) {
            m_out_queue.emplace_back(u.user->addr, msg);
        }
    }

    void tick();

    void set_error_callback(message_callback_t &&fun) {
        m_error_callback = std::move(fun);
    }

private:
    game_user *find_user(const sdlnet::ip_address &addr);
    std::list<lobby>::iterator find_lobby(const game_user *u);

    lobby_data make_lobby_data(const lobby &l);
    void send_lobby_update(const lobby &l);

    void handle_message(MESSAGE_TAG(connect), const sdlnet::ip_address &addr, const connect_args &value);
    void handle_message(MESSAGE_TAG(lobby_list), const sdlnet::ip_address &addr);
    void handle_message(MESSAGE_TAG(lobby_make), const sdlnet::ip_address &addr, const lobby_info &value);
    void handle_message(MESSAGE_TAG(lobby_edit), const sdlnet::ip_address &addr, const lobby_info &args);
    void handle_message(MESSAGE_TAG(lobby_join), const sdlnet::ip_address &addr, const lobby_join_args &value);
    void handle_message(MESSAGE_TAG(lobby_players), const sdlnet::ip_address &addr);
    void handle_message(MESSAGE_TAG(lobby_leave), const sdlnet::ip_address &addr);
    void handle_message(MESSAGE_TAG(lobby_chat), const sdlnet::ip_address &addr, const lobby_chat_client_args &value);
    void handle_message(MESSAGE_TAG(game_start), const sdlnet::ip_address &addr);
    void handle_message(MESSAGE_TAG(game_action), const sdlnet::ip_address &addr, const banggame::game_action &value);

    std::map<sdlnet::ip_address, game_user> users;
    std::list<lobby> m_lobbies;
    std::list<server_message_pair> m_out_queue;

    banggame::all_cards_t all_cards;

    message_callback_t m_error_callback;

    void print_error(const std::string &msg) {
        if (m_error_callback) m_error_callback(msg);
    }
};

#endif