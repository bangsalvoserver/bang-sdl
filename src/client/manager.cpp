#include "manager.h"

#include <sstream>
#include <iostream>

#include "server/net_options.h"
#include "global_resources.h"

using namespace banggame;

game_manager::game_manager(boost::asio::io_context &ctx, const std::filesystem::path &base_path)
    : m_base_path(base_path)
    , m_ctx(ctx)
{
    m_config.load();
    switch_scene<scene_type::connect>();
}

game_manager::~game_manager() {
    m_config.save();
}

void game_manager::update_net() {
    if (!m_loading && m_con) {
        if (m_con->connected()) {
            while (m_con->incoming_messages()) {
                try {
                    enums::visit_indexed([&](auto && ... args) {
                        handle_message(std::forward<decltype(args)>(args)...);
                    }, m_con->pop_message());
                } catch (const std::exception &error) {
                    add_chat_message(message_type::error, std::string("Error: ") + error.what());
                }
            }
        } else {
            disconnect(m_con->address_string().empty() ? std::string() : _("ERROR_DISCONNECTED"));
        }
    }
}

void game_manager::connect(const std::string &host) {
    if (host.empty()) {
        add_chat_message(message_type::error, _("ERROR_NO_ADDRESS"));
        return;
    }
    
    m_loading = true;
    m_con = connection_type::make(m_ctx);
    
    m_con->connect(host, banggame::server_port, [this, host](const boost::system::error_code &ec) {
        m_loading = false;
        if (!ec) {
            m_con->start();
            add_message<client_message_type::connect>(m_config.user_name, binary::serialize(m_config.profile_image_data.get_surface()));
        } else {
            m_con->disconnect();
            if (ec != boost::asio::error::operation_aborted) {
                add_chat_message(message_type::error, ec.message());
            }
        }
    });

    switch_scene<scene_type::loading>()->init(host);
}

void game_manager::disconnect(const std::string &message) {
    if (m_con) {
        m_con->disconnect();
        m_con.reset();
    }

    if (m_listenserver) {
        m_listenserver.reset();
    }

    m_users.clear();

    switch_scene<scene_type::connect>();
    if (!message.empty()) {
        add_chat_message(message_type::error, message);
    }
}


void game_manager::resize(int width, int height) {
    m_width = width;
    m_height = height;

    m_scene->resize(m_width, m_height);

    m_chat.set_rect(sdl::rect{
        width - 250,
        height - 400,
        240,
        350
    });
}

static void render_tiled(sdl::renderer &renderer, const sdl::texture &texture, const sdl::rect &dst_rect) {
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
            SDL_RenderCopy(renderer.get(), texture.get_texture(renderer), &from, &to);
        }
    }
}

void game_manager::render(sdl::renderer &renderer) {
    render_tiled(renderer, global_resources::get().texture_background, sdl::rect{0, 0, width(), height()});
    
    m_scene->render(renderer);

    m_chat.render(renderer);
}

void game_manager::handle_event(const sdl::event &event) {
    if (!widgets::event_handler::handle_events(event)) {
        m_scene->handle_event(event);
    }
}

void game_manager::enable_chat() {
    m_chat.enable();
}

void game_manager::add_chat_message(message_type type, const std::string &message) {
    m_chat.add_message(type, message);
}

bool game_manager::start_listenserver() {
    m_listenserver = std::make_unique<bang_server>(m_ctx, m_base_path);
    m_listenserver->set_message_callback([this](const std::string &msg) {
        add_chat_message(message_type::server_log, std::string("SERVER: ") + msg); 
    });
    m_listenserver->set_error_callback([this](const std::string &msg) {
        add_chat_message(message_type::error, std::string("SERVER: ") + msg);
    });
    if (m_listenserver->start()) {
        return true;
    } else {
        m_listenserver.reset();
        return false;
    }
}

void game_manager::HANDLE_MESSAGE(client_accepted) {
    if (!m_listenserver) {
        auto it = std::ranges::find(m_config.recent_servers, m_con->address_string());
        if (it == m_config.recent_servers.end()) {
            m_config.recent_servers.push_back(m_con->address_string());
        }
    }
    switch_scene<scene_type::lobby_list>();
}

void game_manager::HANDLE_MESSAGE(lobby_error, const std::string &message) {
    add_chat_message(message_type::error, _(message));
}

void game_manager::HANDLE_MESSAGE(lobby_list, const std::vector<lobby_data> &args) {
    m_scene->set_lobby_list(args);
}

void game_manager::HANDLE_MESSAGE(lobby_update, const lobby_data &args) {
    m_scene->handle_lobby_update(args);
}

void game_manager::HANDLE_MESSAGE(lobby_edited, const lobby_info &args) {
    m_scene->set_lobby_info(args);
}

void game_manager::HANDLE_MESSAGE(lobby_players, const std::vector<lobby_player_data> &args) {
    m_scene->clear_users();
    for (const auto &obj : args) {
        const auto &u = m_users.try_emplace(obj.user_id, obj.name, binary::deserialize<sdl::surface>(obj.profile_image)).first->second;
        m_scene->add_user(obj.user_id, u);
    }
}

void game_manager::HANDLE_MESSAGE(lobby_entered, const lobby_entered_args &args) {
    m_lobby_owner_id = args.owner_id;
    m_user_own_id = args.user_id;

    switch_scene<scene_type::lobby>()->init(args);
}

void game_manager::HANDLE_MESSAGE(lobby_joined, const lobby_player_data &args) {
    const auto &u = m_users.try_emplace(args.user_id, args.name, binary::deserialize<sdl::surface>(args.profile_image)).first->second;
    m_scene->add_user(args.user_id, u);
}

void game_manager::HANDLE_MESSAGE(lobby_left, const lobby_left_args &args) {
    if (args.user_id == m_user_own_id) {
        m_users.clear();
        switch_scene<scene_type::lobby_list>();
    } else {
        m_scene->remove_user(args.user_id);
        m_users.erase(args.user_id);
    }
}

void game_manager::HANDLE_MESSAGE(lobby_chat, const lobby_chat_args &args) {
    user_info *info = get_user_info(args.user_id);
    if (info) {
        std::string msg = info->name;
        msg += ": ";
        msg += args.message;

        add_chat_message(message_type::chat, msg);
    }
}

void game_manager::HANDLE_MESSAGE(game_started, const game_started_args &args) {
    switch_scene<scene_type::game>()->init(args);
}

void game_manager::HANDLE_MESSAGE(game_update, const game_update &args) {
    m_scene->handle_game_update(args);
}
