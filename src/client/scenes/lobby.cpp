#include "lobby.h"

#include "../manager.h"

lobby_player_item::lobby_player_item(const lobby_player_data &args)
    : m_name_text("> " + args.name)
    , m_user_id(args.user_id) {}

void lobby_player_item::render(sdl::renderer &renderer, int x, int y) {
    m_name_text.set_point(sdl::point{x, y});
    m_name_text.render(renderer);
}

lobby_scene::lobby_scene(game_manager *parent)
    : scene_base(parent)
    , m_leave_btn("Esci", [parent]{
        parent->add_message<client_message_type::lobby_leave>();
    })
    , m_start_btn("Avvia", [parent]{
        parent->add_message<client_message_type::game_start>();
    })
{

}

void lobby_scene::init(const lobby_entered_args &args) {
    m_lobby_name_text.redraw(args.lobby_name);

    m_owner_id = args.owner_id;
    m_user_id = args.user_id;
    
    parent->add_message<client_message_type::lobby_players>();
}

void lobby_scene::render(sdl::renderer &renderer) {
    if (m_lobby_name_text) {
        sdl::rect rect = m_lobby_name_text.get_rect();
        rect.x = (m_width - rect.w) / 2;
        rect.y = 60;
        m_lobby_name_text.set_rect(rect);
        m_lobby_name_text.render(renderer);
    }

    int y = 100;
    for (auto &line : m_player_list) {
        line.render(renderer, 100, y);
        y += 40;
    }

    if (m_owner_id && m_owner_id == m_user_id) {
        m_start_btn.set_rect(sdl::rect{100, y, 100, 25});
        m_start_btn.render(renderer);
    }

    m_leave_btn.set_rect(sdl::rect{20, m_height - 45, 100, 25});
    m_leave_btn.render(renderer);
}

void lobby_scene::handle_event(const sdl::event &event) {
    m_leave_btn.handle_event(event);

    if (m_owner_id && m_owner_id == m_user_id) {
        m_start_btn.handle_event(event);
    }
}

void lobby_scene::set_player_list(const std::vector<lobby_player_data> &args) {
    m_player_list.clear();
    for (const auto &arg : args) {
        add_user(arg);
    }
}

void lobby_scene::add_user(const lobby_player_data &args) {
    m_player_list.emplace_back(args);
}

void lobby_scene::remove_user(const lobby_left_args &args) {
    if (args.user_id == m_user_id) {
        parent->switch_scene<scene_type::lobby_list>();
    } else {
        auto it = std::ranges::find(m_player_list, args.user_id, &lobby_player_item::user_id);
        if (it != m_player_list.end()) {
            m_player_list.erase(it);
        }
    }
}

void lobby_scene::add_chat_message(const lobby_chat_args &args) {
    m_messages.push_back(args);
}