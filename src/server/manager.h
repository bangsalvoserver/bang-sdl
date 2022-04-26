#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <json/json.h>

#include "utils/binary_serial.h"

#include "net_options.h"
#include "net_enums.h"

#include "game/game.h"

class game_manager;
class game_user;

using user_map = std::map<int, game_user>;
using user_ptr = user_map::iterator;

struct lobby : lobby_info {
    std::vector<user_ptr> users;
    user_ptr owner;
    lobby_state state;

    banggame::game game;
    void start_game(game_manager &mgr, const banggame::all_cards_t &all_cards);
    void send_updates(game_manager &mgr);
};

using lobby_map = std::map<int, lobby>;
using lobby_ptr = lobby_map::iterator;

struct game_user {
    std::string name;
    std::vector<std::byte> profile_image;
    lobby_ptr in_lobby{};
};

struct server_message_pair {
    int client_id;
    server_message value;
};

template<server_message_type E, typename ... Ts>
server_message make_message(Ts && ... args) {
    return server_message{enums::enum_tag<E>, std::forward<Ts>(args) ...};
}

#define MESSAGE_TAG(name) enums::enum_tag_t<client_message_type::name>
#define HANDLE_MESSAGE(name, ...) handle_message(MESSAGE_TAG(name) __VA_OPT__(,) __VA_ARGS__)\

class game_manager {
public:
    game_manager(const std::filesystem::path &base_path);

    struct invalid_message {};
    
    void handle_message(int client_id, const client_message &msg);
    int pending_messages();
    server_message_pair pop_message();

    void client_disconnected(int client_id);
    bool client_validated(int client_id) const;

    template<server_message_type E, typename ... Ts>
    void send_message(int client_id, Ts && ... args) {
        m_out_queue.emplace_back(client_id, make_message<E>(std::forward<Ts>(args) ... ));
    }

    template<server_message_type E, typename ... Ts>
    void broadcast_message(const lobby &lobby, Ts && ... args) {
        auto msg = make_message<E>(std::forward<Ts>(args) ... );
        for (user_ptr it : lobby.users) {
            m_out_queue.emplace_back(it->first, msg);
        }
    }

    void tick();

private:
    lobby_data make_lobby_data(lobby_ptr it);
    void send_lobby_update(lobby_ptr it);

    void HANDLE_MESSAGE(connect,        int client_id, const connect_args &value);
    void HANDLE_MESSAGE(lobby_list,     user_ptr user);
    void HANDLE_MESSAGE(lobby_make,     user_ptr user, const lobby_info &value);
    void HANDLE_MESSAGE(lobby_edit,     user_ptr user, const lobby_info &args);
    void HANDLE_MESSAGE(lobby_join,     user_ptr user, const lobby_join_args &value);
    void HANDLE_MESSAGE(lobby_leave,    user_ptr user);
    void HANDLE_MESSAGE(lobby_chat,     user_ptr user, const lobby_chat_client_args &value);
    void HANDLE_MESSAGE(lobby_return,   user_ptr user);
    void HANDLE_MESSAGE(game_start,     user_ptr user);
    void HANDLE_MESSAGE(game_action,    user_ptr user, const banggame::game_action &value);

    std::map<int, game_user> users;
    lobby_map m_lobbies;

    int m_lobby_counter = 0;

    std::deque<server_message_pair> m_out_queue;

    banggame::all_cards_t all_cards;
};

#endif