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
#include "wsconnection.h"

#include "net/messages.h"

#include <process.hpp>

static constexpr std::chrono::seconds accept_timeout{5};

class client_manager : private net::wsconnection {
public:
    client_manager(sdl::window &window, sdl::renderer &renderer, const std::filesystem::path &base_path);
    ~client_manager();

    void refresh_layout();

    void tick(duration_type time_elapsed);
    void render(sdl::renderer &renderer);

    void handle_event(const sdl::event &event);

    template<banggame::client_message_type E>
    void add_message(auto && ... args) {
        push_message(json::serialize(banggame::client_message{enums::enum_tag<E>, FWD(args) ...}).dump());
    }

    void connect(const std::string &host);
    void disconnect();

    template<std::derived_from<scene_base> T>
    void switch_scene(auto && ... args) {
        m_chat.disable();
        m_scene = std::make_unique<T>(this, FWD(args) ...);
        refresh_layout();
    }

    config &get_config() {
        return m_config;
    }

    sdl::window &get_window() {
        return m_window;
    }

    sdl::renderer &get_renderer() {
        return m_renderer;
    }

    const std::filesystem::path &get_base_path() const { return m_base_path; }

    sdl::rect get_rect() const {
        sdl::rect rect{};
        SDL_GetWindowSize(m_window.get(), &rect.w, &rect.h);
        return rect;
    }

    int width() const { return get_rect().w; }
    int height() const { return get_rect().h; }

    int get_user_own_id() const { return m_config.user_id; }
    int get_lobby_owner_id() const { return m_lobby_owner_id; }

    void client_accepted(const banggame::client_accepted_args &args, const std::string &address);

    std::filesystem::path get_listenserver_path() const;
    bool is_listenserver_present() const;
    void start_listenserver();
    void stop_listenserver();

    banggame::user_info *get_user_info(int id) {
        auto it = m_users.find(id);
        if (it != m_users.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

    void enable_chat();

    sdl::texture browse_propic();
    void reset_propic();
    void send_user_edit();

    void add_chat_message(message_type type, const std::string &message);

protected:
    void on_open() override;
    void on_close() override;
    void on_message(const std::string &msg) override;

private:
    void handle_message(SRV_TAG(ping));
    void handle_message(SRV_TAG(lobby_error), const std::string &message);
    void handle_message(SRV_TAG(lobby_owner), const banggame::user_id_args &args);
    void handle_message(SRV_TAG(lobby_entered), const banggame::lobby_entered_args &args);
    void handle_message(SRV_TAG(lobby_add_user), const banggame::user_info_id_args &args);
    void handle_message(SRV_TAG(lobby_remove_user), const banggame::user_id_args &args);
    void handle_message(SRV_TAG(lobby_update), const banggame::lobby_data &args);
    void handle_message(SRV_TAG(lobby_removed), const banggame::lobby_id_args &args);
    void handle_message(SRV_TAG(lobby_chat), const banggame::lobby_chat_args &args);
    void handle_message(SRV_TAG(game_started));

private:
    sdl::window &m_window;
    sdl::renderer &m_renderer;

    std::filesystem::path m_base_path;
    config m_config;

    std::unique_ptr<scene_base> m_scene;

    chat_ui m_chat{this};

    int m_lobby_owner_id = 0;

private:
    std::atomic<bool> m_connection_open = false;
    std::atomic<bool> m_connection_closed = false;

    std::optional<duration_type> m_accept_timer;

    std::unique_ptr<TinyProcessLib::Process> m_listenserver;
    std::thread m_listenserver_thread;

    std::map<int, banggame::user_info> m_users;
    std::vector<banggame::lobby_data> m_lobbies;
    friend struct bang_connection;
};

#endif