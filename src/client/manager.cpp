#include "manager.h"

#include <sstream>
#include <iostream>

#include "common/options.h"

using namespace banggame;

game_manager::game_manager() {
    switch_scene<scene_type::connect>();
}

game_manager::~game_manager() {
    if (m_scene) {
        delete m_scene;
    }
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
            std::stringstream ss(sock.recv_string());

            Json::Value json_value;
            ss >> json_value;

            auto msg = enums::from_string<server_message_type>(json_value["type"].asString());
            if (msg != enums::invalid_enum_v<server_message_type>) {
                lut[enums::indexof(msg)](*this, json_value["value"]);
            }
        } catch (sdlnet::socket_disconnected) {
            disconnect();
        } catch (Json::Exception) {
            // ignore
        } catch (const std::exception &error) {
            std::cerr << "Errore (" << error.what() << ")\n";
        }
    }
    if (sock.isopen()) {
        while (!m_out_queue.empty()) {
            std::stringstream ss;
            ss << m_out_queue.front();
            sock.send_string(ss.str());
            m_out_queue.pop_front();
        }
    }
}

void game_manager::connect(const std::string &host) {
    try {
        sock.open(sdlnet::ip_address(host, banggame::server_port));
        sock_set.add(sock);
        switch_scene<scene_type::lobby_list>();
    } catch (const sdlnet::net_error &e) {
        if (auto *s = dynamic_cast<connect_scene *>(m_scene)) {
            s->show_error(e.what());
        }
    }
}

void game_manager::disconnect() {
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

void game_manager::handle_event(const SDL_Event &event) {
    m_scene->handle_event(event);
}

void game_manager::handle_message(enums::enum_constant<server_message_type::error_message>, const error_message &args) {
    std::cout << args.message << '\n';
}

void game_manager::handle_message(enums::enum_constant<server_message_type::lobby_list>, const std::vector<lobby_data> &args) {
    if (auto *s = dynamic_cast<lobby_list_scene *>(m_scene)) {
        s->set_lobby_list(args);
    }
}

void game_manager::handle_message(enums::enum_constant<server_message_type::lobby_players>, const std::vector<lobby_player_data> &args) {
    if (auto *s = dynamic_cast<lobby_scene *>(m_scene)) {
        s->set_player_list(args);
    }
}

void game_manager::handle_message(enums::enum_constant<server_message_type::lobby_entered>, const lobby_entered_args &args) {
    switch_scene<scene_type::lobby>()->init(args);
}

void game_manager::handle_message(enums::enum_constant<server_message_type::lobby_joined>, const lobby_player_data &args) {
    if (auto *s = dynamic_cast<lobby_scene *>(m_scene)) {
        s->add_user(args);
    }
}

void game_manager::handle_message(enums::enum_constant<server_message_type::lobby_left>, const lobby_left_args &args) {
    if (auto *s = dynamic_cast<lobby_scene *>(m_scene)) {
        s->remove_user(args);
    }
}

void game_manager::handle_message(enums::enum_constant<server_message_type::lobby_deleted>, const lobby_deleted_args &args) {
    switch_scene<scene_type::lobby_list>();
}

void game_manager::handle_message(enums::enum_constant<server_message_type::lobby_chat>, const lobby_chat_args &args) {
    if (auto *s = dynamic_cast<lobby_scene *>(m_scene)) {
        s->add_chat_message(args);
    } else if (auto *s = dynamic_cast<banggame::game_scene *>(m_scene)) {
        s->add_chat_message(args);
    }
}

void game_manager::handle_message(enums::enum_constant<server_message_type::game_started>) {
    switch_scene<scene_type::game>();
}

void game_manager::handle_message(enums::enum_constant<server_message_type::game_update>, const game_update &args) {
    if (auto *s = dynamic_cast<banggame::game_scene *>(m_scene)) {
        s->handle_update(args);
    }
}
