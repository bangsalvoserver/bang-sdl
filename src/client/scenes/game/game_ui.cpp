#include "game_ui.h"
#include "game.h"

#include "../../manager.h"

using namespace banggame;
using namespace enums::flag_operators;

game_ui::game_ui(game_scene *parent)
    : parent(parent)
    , m_chat(parent->parent)
    , m_pass_btn(_("GAME_PASS"), [&target = parent->m_target] {
        target.on_click_pass_turn();
    })
    , m_resolve_btn(_("GAME_RESOLVE"), [&target = parent->m_target] {
        target.on_click_resolve();
    })
    , m_sell_beer_btn(_("GAME_SELL_BEER"), [&target = parent->m_target] {
        target.on_click_sell_beer();
    })
    , m_discard_black_btn(_("GAME_DISCARD_BLACK"), [&target = parent->m_target] {
        target.on_click_discard_black();
    })
    , m_leave_btn(_("BUTTON_EXIT"), [&mgr = *parent->parent] {
        mgr.add_message<client_message_type::lobby_leave>();
    })
    , m_restart_btn(_("GAME_RESTART"), [&mgr = *parent->parent] {
        mgr.add_message<client_message_type::game_start>();
    }) {}

void game_ui::resize(int width, int height) {
    m_chat.resize(width, height);
    
    m_pass_btn.set_rect(sdl::rect{340, height - 50, 100, 25});
    m_resolve_btn.set_rect(sdl::rect{450, height - 50, 100, 25});
    m_sell_beer_btn.set_rect(sdl::rect{560, height - 50, 100, 25});
    m_discard_black_btn.set_rect(sdl::rect{670, height - 50, 100, 25});

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_restart_btn.set_rect(sdl::rect{140, 20, 100, 25});
}

void game_ui::render(sdl::renderer &renderer) {
    m_chat.render(renderer);

    m_pass_btn.render(renderer);
    m_resolve_btn.render(renderer);
    m_sell_beer_btn.render(renderer);
    m_discard_black_btn.render(renderer);
    
    m_leave_btn.render(renderer);
    m_restart_btn.render(renderer);

    sdl::rect status_rect = m_status_text.get_rect();
    status_rect.x = (parent->parent->width() - status_rect.w) / 2;
    status_rect.y = (parent->parent->height() - status_rect.h) - sizes::status_text_y_distance;
    m_status_text.set_rect(status_rect);
    m_status_text.render(renderer);

    if (m_error_timeout > 0) {
        sdl::rect error_rect = m_error_text.get_rect();
        error_rect.x = (parent->parent->width() - error_rect.w) / 2;
        error_rect.y = (parent->parent->height() - error_rect.h) - sizes::error_msg_y_distance;
        m_error_text.set_rect(error_rect);
        m_error_text.render(renderer);
        --m_error_timeout;
    }
}

void game_ui::set_button_flags(play_card_flags flags) {
    m_sell_beer_btn.set_toggled(bool(flags & play_card_flags::sell_beer));
    m_discard_black_btn.set_toggled(bool(flags & play_card_flags::discard_black));
}

void game_ui::add_message(const std::string &message) {
    m_chat.add_message(message);
}

void game_ui::show_error(const std::string &message) {
    m_error_text.redraw(message);
    m_error_timeout = 200;
}