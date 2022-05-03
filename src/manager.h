#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <list>
#include <map>
#include <memory>

#include "game/net_enums.h"
#include "server/server.h"

#include "config.h"
#include "user_info.h"
#include "intl.h"
#include "chat_ui.h"

#include "scenes/scene_base.h"

#define HANDLE_SRV_MESSAGE(name, ...) handle_message(enums::enum_tag_t<banggame::server_message_type::name> __VA_OPT__(,) __VA_ARGS__)

class client_manager;

struct bang_listenserver : banggame::bang_server<bang_listenserver> {
    using base = banggame::bang_server<bang_listenserver>;

    client_manager &parent;

    bang_listenserver(client_manager &parent, boost::asio::io_context &ctx)
        : base::bang_server(ctx)
        , parent(parent) {}

    void print_message(const std::string &message);
    void print_error(const std::string &message);
};

using bang_client_messages = net::message_types<banggame::server_message, banggame::client_message, banggame::bang_header>;
struct bang_connection : net::connection<bang_connection, bang_client_messages> {
    using base = net::connection<bang_connection, bang_client_messages>;

    client_manager &parent;

    util::tsqueue<banggame::server_message> m_in_queue;

    bang_connection(client_manager &parent, boost::asio::io_context &ctx)
        : base::connection(ctx)
        , parent(parent) {}
    
    void on_receive_message(banggame::server_message msg) {
        m_in_queue.push_back(std::move(msg));
    }

    std::optional<banggame::server_message> pop_message() {
        return m_in_queue.pop_front();
    }

    void on_disconnect();
    void on_error(const std::error_code &ec);
};

class client_manager {
public:
    client_manager(sdl::window &window, boost::asio::io_context &ctx, const std::filesystem::path &base_path);
    ~client_manager();

    void refresh_layout();

    void render(sdl::renderer &renderer);

    void handle_event(const sdl::event &event);

    template<banggame::client_message_type E, typename ... Ts>
    void add_message(Ts && ... args) {
        if (m_con) {
            m_con->push_message(enums::enum_tag<E>, std::forward<Ts>(args) ...);
        }
    }

    void update_net();

    void connect(const std::string &host);
    void disconnect();

    template<std::derived_from<scene_base> T, typename ... Ts>
    void switch_scene(Ts && ... args) {
        m_chat.disable();
        m_scene = std::make_unique<T>(this, std::forward<Ts>(args) ...);
        refresh_layout();
    }

    config &get_config() {
        return m_config;
    }

    sdl::window &get_window() {
        return m_window;
    }

    const std::filesystem::path &get_base_path() const { return m_base_path; }

    sdl::rect get_rect() const {
        sdl::rect rect{};
        SDL_GetWindowSize(m_window.get(), &rect.w, &rect.h);
        return rect;
    }

    int width() const { return get_rect().w; }
    int height() const { return get_rect().h; }

    int get_user_own_id() const { return m_user_own_id; }
    int get_lobby_owner_id() const { return m_lobby_owner_id; }

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
    void HANDLE_SRV_MESSAGE(client_accepted, const banggame::client_accepted_args &args);
    void HANDLE_SRV_MESSAGE(lobby_error, const std::string &message);
    void HANDLE_SRV_MESSAGE(lobby_update, const banggame::lobby_data &args);
    void HANDLE_SRV_MESSAGE(lobby_edited, const banggame::lobby_info &args);
    void HANDLE_SRV_MESSAGE(lobby_entered, const banggame::lobby_entered_args &args);
    void HANDLE_SRV_MESSAGE(lobby_add_user, const banggame::lobby_add_user_args &args);
    void HANDLE_SRV_MESSAGE(lobby_remove_user, const banggame::lobby_remove_user_args &args);
    void HANDLE_SRV_MESSAGE(lobby_chat, const banggame::lobby_chat_args &args);
    void HANDLE_SRV_MESSAGE(game_started, const banggame::game_options &options);
    void HANDLE_SRV_MESSAGE(game_update, const banggame::game_update &args);

private:
    sdl::window &m_window;

    std::filesystem::path m_base_path;
    config m_config;

    std::unique_ptr<scene_base> m_scene;

    chat_ui m_chat{this};

    int m_user_own_id = 0;
    int m_lobby_owner_id = 0;

private:
    boost::asio::io_context &m_ctx;

    bang_connection::pointer m_con;

    boost::asio::basic_waitable_timer<std::chrono::system_clock> m_accept_timer;

    std::map<int, user_info> m_users;
    
    std::unique_ptr<bang_listenserver> m_listenserver;

    friend struct bang_connection;
};

#endif