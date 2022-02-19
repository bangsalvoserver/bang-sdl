#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <list>
#include <map>
#include <memory>

#include "server/server.h"
#include "server/net_enums.h"

#include "scenes/connect.h"
#include "scenes/loading.h"
#include "scenes/lobby_list.h"
#include "scenes/lobby.h"
#include "game/game.h"

#include "config.h"
#include "user_info.h"
#include "intl.h"
#include "chat_ui.h"

#include "utils/connection.h"

DEFINE_ENUM_TYPES(scene_type,
    (connect, connect_scene)
    (loading, loading_scene)
    (lobby_list, lobby_list_scene)
    (lobby, lobby_scene)
    (game, banggame::game_scene)
)

#define HANDLE_MESSAGE(name, ...) handle_message(enums::enum_constant<server_message_type::name> __VA_OPT__(,) __VA_ARGS__)

class game_manager {
public:
    game_manager(const std::filesystem::path &base_path);
    ~game_manager();

    void resize(int width, int height);

    void render(sdl::renderer &renderer);

    void handle_event(const sdl::event &event);

    template<client_message_type E, typename ... Ts>
    void add_message(Ts && ... args) {
        if (m_con) {
            m_con->push_message(enums::enum_constant<E>{}, std::forward<Ts>(args) ...);
        }
    }

    void update_net();

    void connect(const std::string &host);
    void disconnect(const std::string &message);

    template<scene_type E>
    auto *switch_scene() {
        m_chat.disable();
        using type = enums::enum_type_t<E>;
        m_scene = std::make_unique<type>(this);
        m_scene->resize(m_width, m_height);
        return static_cast<type *>(m_scene.get());
    }

    config &get_config() {
        return m_config;
    }

    const std::filesystem::path &get_base_path() const { return m_base_path; }

    int width() const noexcept { return m_width; }
    int height() const noexcept { return m_height; }

    int get_user_own_id() const noexcept { return m_user_own_id; }
    int get_lobby_owner_id() const noexcept { return m_lobby_owner_id; }

    bool start_listenserver();

    user_info *get_user_info(int id) {
        auto it = m_users.find(id);
        if (it != m_users.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

    void enable_chat();

    void add_chat_message(message_type type, const std::string &message);

private:
    void HANDLE_MESSAGE(client_accepted);
    void HANDLE_MESSAGE(lobby_error, const std::string &message);
    void HANDLE_MESSAGE(lobby_list, const std::vector<lobby_data> &args);
    void HANDLE_MESSAGE(lobby_update, const lobby_data &args);
    void HANDLE_MESSAGE(lobby_edited, const lobby_info &args);
    void HANDLE_MESSAGE(lobby_entered, const lobby_entered_args &args);
    void HANDLE_MESSAGE(lobby_players, const std::vector<lobby_player_data> &args);
    void HANDLE_MESSAGE(lobby_joined, const lobby_player_data &args);
    void HANDLE_MESSAGE(lobby_left, const lobby_left_args &args);
    void HANDLE_MESSAGE(lobby_chat, const lobby_chat_args &args);
    void HANDLE_MESSAGE(game_started, const game_started_args &args);
    void HANDLE_MESSAGE(game_update, const banggame::game_update &args);

private:
    std::filesystem::path m_base_path;
    config m_config;

    sdl::texture m_background;
    std::unique_ptr<scene_base> m_scene;

    chat_ui m_chat{this};

    int m_width;
    int m_height;

    int m_user_own_id = 0;
    int m_lobby_owner_id = 0;

private:
    boost::asio::io_context m_ctx;

    using connection_type = net::connection<server_message, client_message, banggame::bang_header>;
    connection_type::pointer m_con;
    std::string m_connected_address;
    std::atomic<bool> m_loading = false;

    std::map<int, user_info> m_users;
    
    std::unique_ptr<bang_server> m_listenserver;

    std::jthread m_ctx_thread;
};

#endif