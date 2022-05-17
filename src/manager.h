#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <list>
#include <map>
#include <memory>

#include "scenes/scene_base.h"

#include "config.h"
#include "intl.h"
#include "chat_ui.h"
#include "image_serial.h"

#include "game/messages.h"

#include "net/wsconnection.h"

#include "subprocess.h"

struct user_info {
    std::string name;
    sdl::texture profile_image;

    user_info(std::string name, sdl::surface &&data)
        : name(std::move(name))
        , profile_image(std::move(data)) {}
};

#define HANDLE_SRV_MESSAGE(name, ...) handle_message(enums::enum_tag_t<banggame::server_message_type::name> __VA_OPT__(,) __VA_ARGS__)

class client_manager;

struct bang_connection : net::wsconnection<bang_connection, banggame::server_message, banggame::client_message> {
    using base = net::wsconnection<bang_connection, banggame::server_message, banggame::client_message>;
    using client_handle = typename base::client_handle;

    client_manager &parent;

    asio::basic_waitable_timer<std::chrono::system_clock> m_accept_timer;
    static constexpr std::chrono::seconds accept_timeout{5};

    bang_connection(client_manager &parent, asio::io_context &ctx)
        : base(ctx), parent(parent), m_accept_timer(ctx) {}

    void on_open();
    void on_close();

    void on_message(const banggame::server_message &msg);
};

class client_manager {
public:
    client_manager(sdl::window &window, asio::io_context &ctx, const std::filesystem::path &base_path);
    ~client_manager();

    void refresh_layout();

    void render(sdl::renderer &renderer);

    void handle_event(const sdl::event &event);

    template<banggame::client_message_type E, typename ... Ts>
    void add_message(Ts && ... args) {
        m_con.push_message(enums::enum_tag<E>, std::forward<Ts>(args) ...);
    }

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

    void start_listenserver();

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
    bang_connection m_con;

    subprocess::process m_listenserver;
    asio::basic_waitable_timer<std::chrono::system_clock> m_listenserver_timer;

    std::map<int, user_info> m_users;
    friend struct bang_connection;
};

#endif