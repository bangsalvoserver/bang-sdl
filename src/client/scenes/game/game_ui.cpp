#include "game_ui.h"
#include "game.h"

#include "../../manager.h"

using namespace banggame;

game_ui::game_ui(game_scene *parent)
    : parent(parent)
    , m_chat(parent->parent)
    , m_pass_btn("Passa", [parent] {
        parent->add_pass_action();
    })
    , m_resolve_btn("Risolvi", [parent] {
        parent->add_resolve_action();
    })
    , m_leave_btn("Esci", [parent] {
        parent->parent->add_message<client_message_type::lobby_leave>();
    }) {}

void game_ui::resize(int width, int height) {
    m_chat.resize(width, height);
    
    m_pass_btn.set_rect(sdl::rect{340, height - 50, 100, 25});
    m_resolve_btn.set_rect(sdl::rect{450, height - 50, 100, 25});
    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
}

void game_ui::render(sdl::renderer &renderer) {
    m_chat.render(renderer);

    m_pass_btn.render(renderer);
    m_resolve_btn.render(renderer);
    m_leave_btn.render(renderer);

    sdl::rect status_rect = m_status_text.get_rect();
    status_rect.x = (parent->parent->width() - status_rect.w) / 2;
    status_rect.y = (parent->parent->height() - status_rect.h) - 10;
    m_status_text.set_rect(status_rect);
    m_status_text.render(renderer);

    if (m_error_timeout > 0) {
        sdl::rect error_rect = m_error_text.get_rect();
        error_rect.x = (parent->parent->width() - error_rect.w) / 2;
        error_rect.y = (parent->parent->height() - error_rect.h) - 50;
        m_error_text.set_rect(error_rect);
        m_error_text.render(renderer);
        --m_error_timeout;
    }
}

void game_ui::add_message(const std::string &message) {
    m_chat.add_message(message);
}

void game_ui::show_error(const std::string &message) {
    m_error_text.redraw(message);
    m_error_timeout = 200;
}