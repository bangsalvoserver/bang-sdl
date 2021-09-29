#include "game_ui.h"
#include "game.h"

#include "../../manager.h"

using namespace banggame;

message_line::message_line(message_type type, const std::string &message)
    : m_text(message) {}

void message_line::render(sdl::renderer &renderer, const SDL_Point &pt) {
    m_text.set_point(pt);
    m_text.render(renderer);
}

game_ui::game_ui(game_scene *parent)
    : parent(parent)
    , m_pass_btn("Passa", [parent] {
        parent->add_pass_action();
    })
    , m_resolve_btn("Risolvi", [parent] {
        parent->add_resolve_action();
    })
    , m_leave_btn("Esci", [parent] {
        parent->parent->add_message<client_message_type::lobby_leave>();
        parent->parent->switch_scene<scene_type::lobby_list>();
    }) {}

void game_ui::resize(int width, int height) {
    m_width = width;
    m_height = height;
    
    m_pass_btn.set_rect(SDL_Rect{20, m_height - 50, 100, 25});
    m_resolve_btn.set_rect(SDL_Rect{130, m_height - 50, 100, 25});
    m_leave_btn.set_rect(SDL_Rect{20, 20, 100, 25});
}

void game_ui::render(sdl::renderer &renderer) {
    SDL_Point pt{20, m_height - 80 };
    for (auto &line : m_messages | std::views::reverse) {
        line.render(renderer, pt);
        pt.y -= 20;
    }

    m_pass_btn.render(renderer);
    m_resolve_btn.render(renderer);
    m_leave_btn.render(renderer);
}

void game_ui::handle_event(const SDL_Event &event) {
    m_pass_btn.handle_event(event);
    m_resolve_btn.handle_event(event);
    m_leave_btn.handle_event(event);
}

void game_ui::add_message(message_line::message_type type, const std::string &message) {
    m_messages.emplace_back(type, message);
    if (m_messages.size() > max_messages) {
        m_messages.pop_front();
    }
}