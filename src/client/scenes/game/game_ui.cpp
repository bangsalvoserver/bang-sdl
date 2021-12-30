#include "game_ui.h"
#include "game.h"

#include "../../manager.h"

using namespace banggame;
using namespace enums::flag_operators;

game_ui::game_ui(game_scene *parent)
    : parent(parent)
    , m_chat(parent->parent)
    , m_game_log(sdl::text_list_style{
        .text = {
            .text_ptsize = sdl::chat_log_ptsize
        },
        .align = sdl::text_alignment::right,
        .text_offset = sdl::chat_log_yoffset
    })
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
    })
    , m_error_text(sdl::text_style{
        .text_color = sdl::rgb(sdl::game_error_text_rgb)
    }) {}

void game_ui::resize(int width, int height) {
    m_chat.resize(width, height);
    m_game_log.set_rect(sdl::rect{width - 320, 300, 300, height - 370});
    
    m_pass_btn.set_rect(sdl::rect{340, height - 50, 100, 25});
    m_resolve_btn.set_rect(sdl::rect{450, height - 50, 100, 25});
    m_sell_beer_btn.set_rect(sdl::rect{560, height - 50, 100, 25});
    m_discard_black_btn.set_rect(sdl::rect{670, height - 50, 100, 25});

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_restart_btn.set_rect(sdl::rect{140, 20, 100, 25});
}

void game_ui::render(sdl::renderer &renderer) {
    m_chat.render(renderer);
    m_game_log.render(renderer);

    m_pass_btn.render(renderer);
    m_resolve_btn.render(renderer);
    m_sell_beer_btn.render(renderer);
    m_discard_black_btn.render(renderer);
    
    m_leave_btn.render(renderer);
    m_restart_btn.render(renderer);

    auto draw_status = [&](sdl::stattext &text) {
        if (text.get_value().empty()) return;

        sdl::rect rect = text.get_rect();
        rect.x = (parent->parent->width() - rect.w) / 2;
        rect.y = (parent->parent->height() - rect.h) - sizes::status_text_y_distance;
        renderer.set_draw_color(sdl::rgba(sizes::status_text_background_rgba));
        renderer.fill_rect(sdl::rect{rect.x - 5, rect.y - 2, rect.w + 10, rect.h + 4});
        text.set_rect(rect);
        text.render(renderer);
    };

    if (m_error_timeout > 0) {
        draw_status(m_error_text);
        --m_error_timeout;
    } else {
        draw_status(m_status_text);
    }
}

void game_ui::set_button_flags(play_card_flags flags) {
    m_sell_beer_btn.set_toggled(bool(flags & play_card_flags::sell_beer));
    m_discard_black_btn.set_toggled(bool(flags & play_card_flags::discard_black));
}

void game_ui::add_message(const std::string &message) {
    m_chat.add_message(message);
}

void game_ui::add_game_log(const std::string &message) {
    m_game_log.add_message(message);
}

void game_ui::show_error(const std::string &message) {
    m_error_text.redraw(message);
    m_error_timeout = 200;
}