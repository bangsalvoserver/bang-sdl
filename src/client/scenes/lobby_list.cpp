#include "lobby_list.h"

#include "../manager.h"

lobby_line::lobby_line(lobby_list_scene *parent, const lobby_data &args)
    : parent(parent)
    , m_name_text(args.name)
    , m_players_text([&]{
        return std::to_string(args.num_players) + '/' + std::to_string(args.max_players);
    }())
    , m_state_text(std::string(enums::to_string(args.state)))
    , m_join_btn("Entra", [parent, args] {
        parent->do_join(args.lobby_id);
    }) {}

void lobby_line::render(sdl::renderer &renderer, const SDL_Rect &rect) {
    m_name_text.set_point(SDL_Point{rect.x, rect.y});
    m_name_text.render(renderer);

    m_players_text.set_point(SDL_Point{rect.x + rect.w - 250, rect.y});
    m_players_text.render(renderer);

    m_state_text.set_point(SDL_Point{rect.x + rect.w - 200, rect.y});
    m_state_text.render(renderer);

    m_join_btn.set_rect(SDL_Rect{rect.x + rect.w - 100, rect.y, 100, rect.h});
    m_join_btn.render(renderer);
}

void lobby_line::handle_event(const SDL_Event &event) {
    m_join_btn.handle_event(event);
}

lobby_list_scene::lobby_list_scene(game_manager *parent)
    : scene_base(parent)
    , m_disconnect_btn("Disconnetti", [parent] {
        parent->disconnect();
    })
    , m_refresh_btn("Aggiorna", [this] {
        refresh();
    })
    , m_make_lobby_btn("Crea lobby", [parent] {
        parent->switch_scene<scene_type::make_lobby>();
    })
    , m_username_label("Nome utente:")
{
    refresh();
}

void lobby_list_scene::render(sdl::renderer &renderer, int w, int h) {
    auto label_rect = m_username_label.get_rect();
    label_rect.x = 100;
    label_rect.y = 50 + (25 - label_rect.h) / 2;
    m_username_label.set_rect(label_rect);
    m_username_label.render(renderer);
    
    m_username_box.set_rect(SDL_Rect{100 + label_rect.w + 10, 50, w - 210 - label_rect.w, 25});
    m_username_box.render(renderer);
    
    int y = 100;
    for (auto &line : m_lobby_lines) {
        line.render(renderer, SDL_Rect{100, y, w - 200, 25});
        y += 40;
    }

    m_refresh_btn.set_rect(SDL_Rect{100, y, 100, 25});
    m_refresh_btn.render(renderer);

    m_make_lobby_btn.set_rect(SDL_Rect{210, y, 100, 25});
    m_make_lobby_btn.render(renderer);

    m_disconnect_btn.set_rect(SDL_Rect{20, h - 45, 100, 25});
    m_disconnect_btn.render(renderer);
}

void lobby_list_scene::handle_event(const SDL_Event &event) {
    m_username_box.handle_event(event);

    for (auto &line : m_lobby_lines) {
        line.handle_event(event);
    }

    m_refresh_btn.handle_event(event);
    m_make_lobby_btn.handle_event(event);
    m_disconnect_btn.handle_event(event);
}

void lobby_list_scene::refresh() {
    parent->add_message<client_message_type::lobby_list>();
}

void lobby_list_scene::do_join(int lobby_id) {
    parent->add_message<client_message_type::lobby_join>(lobby_id, m_username_box.get_value());
}

void lobby_list_scene::set_lobby_list(const std::vector<lobby_data> &args) {
    m_lobby_lines.clear();
    for (const auto &line : args) {
        m_lobby_lines.emplace_back(this, line);
    }
}