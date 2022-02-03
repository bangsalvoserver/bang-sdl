#include "manager.h"

#include <sstream>
#include <iostream>

#include "utils/message_header.h"
#include "utils/binary_serial.h"

#include "common/options.h"

DECLARE_RESOURCE(background_png)

using namespace banggame;

game_manager::game_manager(const std::string &config_filename)
    : m_background(sdl::surface(GET_RESOURCE(background_png)))
    , m_config(config_filename) {
    switch_scene<scene_type::connect>();
}

game_manager::~game_manager() {
    m_config.save();
}

void game_manager::update_net() {
    while (sock.isopen() && sock_set.check(0)) {
        try {
            enums::visit_indexed([&](auto && ... args) {
                handle_message(std::forward<decltype(args)>(args)...);
            }, binary::deserialize<server_message>(recv_message_bytes(sock)));
        } catch (sdlnet::socket_disconnected) {
            disconnect();
        } catch (const binary::read_error &error) {
            std::cerr << "Deserialization error: " << error.what() << '\n';
        } catch (const std::exception &error) {
            std::cerr << "Error (" << error.what() << ")\n";
        }
    }
    if (sock.isopen()) {
        while (!m_out_queue.empty()) {
            send_message_bytes(sock, binary::serialize(m_out_queue.front()));
            m_out_queue.pop_front();
        }
    }
}

static std::vector<std::byte> load_profile_image(const std::string &filename) {
    if (filename.empty()) return {};

    try {
        return encode_profile_image(sdl::surface(resource(filename)));
    } catch (const std::runtime_error &e) {
        return {};
    }
}

void game_manager::connect(const std::string &host) {
    try {
        sock.open(sdlnet::ip_address(host, banggame::server_port));
        sock_set.add(sock);
        connected_ip = host;

        add_message<client_message_type::connect>(m_config.user_name, load_profile_image(m_config.profile_image));
    } catch (const sdlnet::net_error &e) {
        m_scene->show_error(e.what());
    }
}

void game_manager::disconnect() {
    m_users.clear();
    sock_set.erase(sock);
    sock.close();
    switch_scene<scene_type::connect>()->show_error(_("ERROR_DISCONNECTED"));
    if (m_listenserver) {
        m_listenserver.reset();
    }
}


void game_manager::resize(int width, int height) {
    m_width = width;
    m_height = height;

    m_scene->resize(m_width, m_height);
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
    render_tiled(renderer, m_background, sdl::rect{0, 0, width(), height()});
    
    m_scene->render(renderer);
}

void game_manager::handle_event(const sdl::event &event) {
    if (!sdl::event_handler::handle_events(event)) {
        m_scene->handle_event(event);
    }
}

bool game_manager::start_listenserver() {
    m_listenserver = std::make_unique<bang_server>();
    m_listenserver->set_message_callback([this](const std::string &msg) {
        m_scene->add_chat_message(std::string("SERVER: ") + msg); 
    });
    m_listenserver->set_error_callback([this](const std::string &msg) {
        m_scene->show_error(std::string("SERVER: ") + msg);
    });
    if (m_listenserver->start()) {
        return true;
    } else {
        m_listenserver.reset();
        return false;
    }
}

void game_manager::handle_message(MESSAGE_TAG(client_accepted)) {
    if (!connected_ip.empty() && !m_listenserver) {
        auto it = std::ranges::find(m_config.recent_servers, connected_ip);
        if (it == m_config.recent_servers.end()) {
            m_config.recent_servers.push_back(connected_ip);
        }
    }
    switch_scene<scene_type::lobby_list>();
}

void game_manager::handle_message(MESSAGE_TAG(lobby_list), const std::vector<lobby_data> &args) {
    m_scene->set_lobby_list(args);
}

void game_manager::handle_message(MESSAGE_TAG(lobby_update), const lobby_data &args) {
    m_scene->handle_lobby_update(args);
}

void game_manager::handle_message(MESSAGE_TAG(lobby_edited), const lobby_info &args) {
    m_scene->set_lobby_info(args);
}

void game_manager::handle_message(MESSAGE_TAG(lobby_players), const std::vector<lobby_player_data> &args) {
    m_scene->clear_users();
    for (const auto &obj : args) {
        const auto &u = m_users.try_emplace(obj.user_id, obj.name, obj.profile_image).first->second;
        m_scene->add_user(obj.user_id, u);
    }
}

void game_manager::handle_message(MESSAGE_TAG(lobby_entered), const lobby_entered_args &args) {
    m_lobby_owner_id = args.owner_id;
    m_user_own_id = args.user_id;

    switch_scene<scene_type::lobby>()->init(args);
}

void game_manager::handle_message(MESSAGE_TAG(lobby_joined), const lobby_player_data &args) {
    const auto &u = m_users.try_emplace(args.user_id, args.name, args.profile_image).first->second;
    m_scene->add_user(args.user_id, u);
}

void game_manager::handle_message(MESSAGE_TAG(lobby_left), const lobby_left_args &args) {
    if (args.user_id == m_user_own_id) {
        m_users.clear();
        switch_scene<scene_type::lobby_list>();
    } else {
        m_scene->remove_user(args.user_id);
        m_users.erase(args.user_id);
    }
}

void game_manager::handle_message(MESSAGE_TAG(lobby_chat), const lobby_chat_args &args) {
    user_info *info = get_user_info(args.user_id);
    if (info) {
        std::string msg = info->name;
        msg += ": ";
        msg += args.message;

        m_scene->add_chat_message(msg);
    }
}

void game_manager::handle_message(MESSAGE_TAG(game_started), const game_started_args &args) {
    switch_scene<scene_type::game>()->init(args);
}

void game_manager::handle_message(MESSAGE_TAG(game_update), const game_update &args) {
    m_scene->handle_game_update(args);
}
