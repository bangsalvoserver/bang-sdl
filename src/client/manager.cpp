#include "manager.h"

#include <sstream>
#include <iostream>

#include "common/options.h"
#include "common/message_header.h"

using namespace banggame;

game_manager::game_manager(const std::string &config_filename) : m_config(config_filename) {
    switch_scene<scene_type::connect>();
}

game_manager::~game_manager() {
    if (m_scene) {
        delete m_scene;
    }
    m_config.save();
}

void game_manager::update_net() {
    constexpr auto lut = []<server_message_type ... Es>(enums::enum_sequence<Es...>) {
        return std::array{ +[](game_manager &mgr, const Json::Value &value) {
            constexpr server_message_type E = Es;
            if constexpr (enums::has_type<E>) {
                mgr.handle_message(enums::enum_constant<E>{}, json::deserialize<enums::enum_type_t<E>>(value));
            } else {
                mgr.handle_message(enums::enum_constant<E>{});
            }
        } ... };
    }(enums::make_enum_sequence<server_message_type>());

    while (sock.isopen() && sock_set.check(0)) {
        try {
            auto header = recv_message_header(sock);
            auto str = recv_message_string(sock, header.length);
            switch (header.type) {
            case message_header::json: {
                util::isviewstream ss(str);
                Json::Value json_value;
                ss >> json_value;

                auto msg = enums::from_string<server_message_type>(json_value["type"].asString());
                if (msg != enums::invalid_enum_v<server_message_type>) {
                    lut[enums::indexof(msg)](*this, json_value["value"]);
                }
                break;
            }
            default:
                break;
            }
        } catch (sdlnet::socket_disconnected) {
            disconnect();
        } catch (const Json::Exception &e) {
            std::cerr << "Errore Json (" << e.what() << ")\n";
        } catch (const std::exception &error) {
            std::cerr << "Errore (" << error.what() << ")\n";
        }
    }
    if (sock.isopen()) {
        while (!m_out_queue.empty()) {
            std::stringstream ss;
            ss << m_out_queue.front();
            std::string str = ss.str();

            send_message_header(sock, message_header{message_header::json, (uint32_t)str.size()});
            send_message_string(sock, str);
            
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
    switch_scene<scene_type::connect>()->show_error("Disconnesso dal server");
}


void game_manager::resize(int width, int height) {
    m_width = width;
    m_height = height;

    m_scene->resize(m_width, m_height);
}

void game_manager::render(sdl::renderer &renderer) {
    renderer.set_draw_color(m_scene->bg_color());
    renderer.render_clear();
    
    m_scene->render(renderer);
}

void game_manager::handle_event(const sdl::event &event) {
    if (!sdl::event_handler::handle_events(event)) {
        m_scene->handle_event(event);
    }
}

void game_manager::handle_message(MESSAGE_TAG(client_accepted)) {
    if (!connected_ip.empty()) {
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

void game_manager::handle_message(MESSAGE_TAG(game_error), const std::string &message) {
    m_scene->show_error(message);
}

void game_manager::handle_message(MESSAGE_TAG(game_update), const game_update &args) {
    m_scene->handle_game_update(args);
}
