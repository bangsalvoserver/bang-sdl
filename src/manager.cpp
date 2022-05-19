#include "manager.h"

#include <sstream>
#include <iostream>
#include <charconv>

#include "game/net_options.h"

#include "media_pak.h"

#include "scenes/connect.h"
#include "scenes/loading.h"
#include "scenes/lobby_list.h"
#include "scenes/lobby.h"
#include "gamescene/game.h"

using namespace banggame;

client_manager::client_manager(sdl::window &window, sdl::renderer &renderer, asio::io_context &ctx, const std::filesystem::path &base_path)
    : m_window(window)
    , m_renderer(renderer)
    , m_base_path(base_path)
    , m_con(*this, ctx)
    , m_listenserver_timer(ctx)
{
    m_config.load();
    switch_scene<connect_scene>();
}

client_manager::~client_manager() {
    m_config.save();
}

void bang_connection::on_open() {
    m_accept_timer.expires_after(accept_timeout);
    m_accept_timer.async_wait([this](const std::error_code &ec) {
        if (!ec) {
            parent.add_chat_message(message_type::error, _("TIMEOUT_EXPIRED"));
            disconnect();
        }
    });

    parent.add_message<banggame::client_message_type::connect>(parent.m_config.user_name, parent.m_config.profile_image_data);
}

void bang_connection::on_close() {
    if (parent.m_listenserver) {
        parent.m_listenserver.abort();
    }
    parent.switch_scene<connect_scene>();
}

void bang_connection::on_message(const server_message &message) {
    try {
        enums::visit_indexed([&](auto && ... args) {
            parent.handle_message(std::forward<decltype(args)>(args)...);
        }, message);
    } catch (const std::exception &error) {
        parent.add_chat_message(message_type::error, fmt::format("Error: {}", error.what()));
    }
}

void client_manager::connect(const std::string &host) {
    if (host.empty()) {
        add_chat_message(message_type::error, _("ERROR_NO_ADDRESS"));
    } else {
        if (host.find(":") == std::string::npos) {
            m_con.connect(fmt::format("{}:{}", host, default_server_port));
        } else {
            m_con.connect(host);
        }

        switch_scene<loading_scene>(_("CONNECTING_TO", host));
    }
}

void client_manager::disconnect() {
    m_con.disconnect();
    m_listenserver_timer.cancel();
}

void client_manager::refresh_layout() {
    m_scene->refresh_layout();

    m_chat.set_rect(sdl::rect{
        width() - 250,
        height() - 400,
        240,
        350
    });
}

static void render_tiled(sdl::renderer &renderer, sdl::texture_ref texture, const sdl::rect &dst_rect) {
    const sdl::rect src_rect = texture.get_rect();

    for (int y=dst_rect.y; y<=dst_rect.w + dst_rect.h + src_rect.h; y+=src_rect.h) {
        for (int x=dst_rect.x; x<=dst_rect.x + dst_rect.w + src_rect.w; x+=src_rect.w) {
            sdl::rect from = src_rect;
            sdl::rect to{x, y, src_rect.w, src_rect.h};
            if (to.x + to.w > dst_rect.x + dst_rect.w) {
                from.w = to.w = dst_rect.x + dst_rect.w - to.x;
            }
            if (to.y + to.h > dst_rect.y + dst_rect.h) {
                from.h = to.h = dst_rect.y + dst_rect.h - to.y;
            }
            SDL_RenderCopy(renderer.get(), texture.get(), &from, &to);
        }
    }
}

void client_manager::render(sdl::renderer &renderer) {
    render_tiled(renderer, media_pak::get().texture_background, sdl::rect{0, 0, width(), height()});
    
    m_scene->render(renderer);

    m_chat.render(renderer);
}

void client_manager::handle_event(const sdl::event &event) {
    if (!widgets::event_handler::handle_events(event)) {
        m_scene->handle_event(event);
    }
}

void client_manager::enable_chat() {
    m_chat.enable();
}

void client_manager::add_chat_message(message_type type, const std::string &message) {
    m_chat.add_message(type, message);
}

void client_manager::start_listenserver() {
    auto server_path = m_base_path / "bangserver";
#ifdef WIN32
    server_path.replace_extension("exe");
#endif
    if (!m_config.server_port) {
        m_config.server_port = default_server_port;
    }
    std::string port_str = std::to_string(m_config.server_port);
    m_listenserver.open(subprocess::arguments{server_path.string(), port_str});
    switch_scene<loading_scene>(_("CREATING_SERVER"));
    m_listenserver_timer.expires_after(std::chrono::seconds{1});
    m_listenserver_timer.async_wait([this](const std::error_code &ec) {
        if (!ec) {
            if (m_listenserver) {
                m_con.connect(fmt::format("localhost:{}", m_config.server_port));
            } else {
                add_chat_message(message_type::error, _("ERROR_CREATING_SERVER"));
                m_listenserver.close();
                m_con.on_close();
            }
        } else {
            m_con.on_close();
        }
    });
}

void client_manager::HANDLE_SRV_MESSAGE(client_accepted, const client_accepted_args &args) {
    if (!m_listenserver) {
        auto it = std::ranges::find(m_config.recent_servers, m_con.address_string());
        if (it == m_config.recent_servers.end()) {
            m_config.recent_servers.push_back(m_con.address_string());
        }
    }
    m_con.m_accept_timer.cancel();
    m_users.clear();
    m_user_own_id = args.user_id;
    switch_scene<lobby_list_scene>();
}

void client_manager::HANDLE_SRV_MESSAGE(lobby_error, const std::string &message) {
    add_chat_message(message_type::error, _(message));
}

void client_manager::HANDLE_SRV_MESSAGE(lobby_update, const lobby_data &args) {
    if (auto scene = dynamic_cast<lobby_list_scene *>(m_scene.get())) {
        scene->handle_lobby_update(args);
    }
}

void client_manager::HANDLE_SRV_MESSAGE(lobby_edited, const lobby_info &args) {
    if (auto scene = dynamic_cast<lobby_scene *>(m_scene.get())) {
        scene->set_lobby_info(args);
    }
}

void client_manager::HANDLE_SRV_MESSAGE(lobby_entered, const lobby_entered_args &args) {
    m_lobby_owner_id = args.owner_id;
    switch_scene<lobby_scene>(args);
}

const user_info &client_manager::add_user(int id, std::string name, const sdl::surface &surface) {
    return m_users[id] = {std::move(name), sdl::texture(get_renderer(), surface)};
}

void client_manager::HANDLE_SRV_MESSAGE(lobby_add_user, const lobby_add_user_args &args) {
    const auto &u = add_user(args.user_id, args.name, args.profile_image);
    add_chat_message(message_type::server_log, _("GAME_USER_CONNECTED", args.name));
    if (auto scene = dynamic_cast<lobby_scene *>(m_scene.get())) {
        scene->add_user(args.user_id, u);
    }
}

void client_manager::HANDLE_SRV_MESSAGE(lobby_remove_user, const lobby_remove_user_args &args) {
    if (args.user_id == m_user_own_id) {
        m_users.clear();
        switch_scene<lobby_list_scene>();
    } else {
        if (auto scene = dynamic_cast<lobby_scene *>(m_scene.get())) {
            scene->remove_user(args.user_id);
        }
        if (auto it = m_users.find(args.user_id); it != m_users.end()) {
            add_chat_message(message_type::server_log, _("GAME_USER_DISCONNECTED", it->second.name));
        }
    }
}

void client_manager::HANDLE_SRV_MESSAGE(lobby_chat, const lobby_chat_args &args) {
    user_info *info = get_user_info(args.user_id);
    if (info) {
        std::string msg = info->name;
        msg += ": ";
        msg += args.message;

        add_chat_message(message_type::chat, msg);
    }
}

void client_manager::HANDLE_SRV_MESSAGE(game_started, const game_options &options) {
    switch_scene<banggame::game_scene>(options);
}

void client_manager::HANDLE_SRV_MESSAGE(game_update, const game_update &args) {
    if (auto scene = dynamic_cast<banggame::game_scene *>(m_scene.get())) {
        scene->handle_game_update(args);
    }
}
