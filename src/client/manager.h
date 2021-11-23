#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <list>
#include <map>

#include "common/net_enums.h"

#include "scenes/connect.h"
#include "scenes/lobby_list.h"
#include "scenes/lobby.h"
#include "scenes/game/game.h"

#include "config.h"
#include "user_info.h"

DEFINE_ENUM_TYPES(scene_type,
    (connect, connect_scene)
    (lobby_list, lobby_list_scene)
    (lobby, lobby_scene)
    (game, banggame::game_scene)
)

#define MESSAGE_TAG(name) enums::enum_constant<server_message_type::name>

class game_manager {
public:
    game_manager(const std::string &config_filename);
    ~game_manager();

    void resize(int width, int height);

    void render(sdl::renderer &renderer);

    void handle_event(const sdl::event &event);

    template<client_message_type E, typename ... Ts>
    void add_message(Ts && ... args) {
        Json::Value message = Json::objectValue;
        message["type"] = json::serialize(E);
        if constexpr (enums::has_type<E>) {
            message["value"] = json::serialize(enums::enum_type_t<E>{ std::forward<Ts>(args) ... });
        }
        m_out_queue.emplace_back(std::move(message));
    }

    void update_net();

    void connect(const std::string &host);
    void disconnect();

    template<scene_type E>
    auto *switch_scene() {
        if (m_scene) {
            delete m_scene;
        }
        auto *scene = new enums::enum_type_t<E>(this);
        scene->resize(m_width, m_height);
        m_scene = scene;
        return scene;
    }

    config &get_config() {
        return m_config;
    }

    int width() const noexcept { return m_width; }
    int height() const noexcept { return m_height; }

    int get_user_own_id() const noexcept { return m_user_own_id; }
    int get_lobby_owner_id() const noexcept { return m_lobby_owner_id; }

    user_info *get_user_info(int id) {
        auto it = m_users.find(id);
        if (it != m_users.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

private:
    void handle_message(MESSAGE_TAG(client_accepted));
    void handle_message(MESSAGE_TAG(lobby_list), const std::vector<lobby_data> &args);
    void handle_message(MESSAGE_TAG(lobby_update), const lobby_data &args);
    void handle_message(MESSAGE_TAG(lobby_edited), const lobby_info &args);
    void handle_message(MESSAGE_TAG(lobby_entered), const lobby_entered_args &args);
    void handle_message(MESSAGE_TAG(lobby_players), const std::vector<lobby_player_data> &args);
    void handle_message(MESSAGE_TAG(lobby_joined), const lobby_player_data &args);
    void handle_message(MESSAGE_TAG(lobby_left), const lobby_left_args &args);
    void handle_message(MESSAGE_TAG(lobby_chat), const lobby_chat_args &args);
    void handle_message(MESSAGE_TAG(game_started), const game_started_args &args);
    void handle_message(MESSAGE_TAG(game_error), const std::string &message);
    void handle_message(MESSAGE_TAG(game_update), const banggame::game_update &args);

    scene_base *m_scene = nullptr;

    std::list<Json::Value> m_out_queue;

    sdlnet::tcp_socket sock;
    sdlnet::socket_set sock_set{1};
    std::string connected_ip;

    std::map<int, user_info> m_users;

    int m_width;
    int m_height;

    int m_user_own_id = 0;
    int m_lobby_owner_id = 0;

    config m_config;
};

#endif