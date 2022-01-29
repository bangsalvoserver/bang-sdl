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
        }
    })
    , m_confirm_btn(_("GAME_CONFIRM"), [&target = parent->m_target] {
        target.on_click_confirm();
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
    m_game_log.set_rect(sdl::rect{width - 220, height - 310, 210, 250});

    int x = 340;
    for (auto &[btn, card] : m_special_btns) {
        btn.set_rect(sdl::rect{x, height - 50, 100, 25});
        x += 110;
    }
    
    m_confirm_btn.set_rect(sdl::rect{x, height - 50, 100, 25});

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_restart_btn.set_rect(sdl::rect{140, 20, 100, 25});
}

void game_ui::render(sdl::renderer &renderer) {
    m_chat.render(renderer);
    m_game_log.render(renderer);

    m_confirm_btn.set_toggled(parent->m_target.can_confirm());
    m_confirm_btn.render(renderer);

    for (auto &[btn, card] : m_special_btns) {
        btn.set_toggled(parent->m_target.is_playing_card(card) || parent->m_target.can_respond_with(card));
        btn.render(renderer);
    }
    
    m_leave_btn.render(renderer);
    m_restart_btn.render(renderer);

    auto draw_status = [&](sdl::stattext &text) {
        if (text.get_value().empty()) return;

        sdl::rect rect = text.get_rect();
        rect.x = (parent->parent->width() - rect.w) / 2;
        rect.y = (parent->parent->height() - rect.h) - sizes::status_text_y_distance;
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

void game_ui::add_special(card_view *card) {
    m_special_btns.emplace_back(std::piecewise_construct,
        std::make_tuple(_(card->name), [&target = parent->m_target, card]{
            target.on_click_scenario_card(card);
        }), std::make_tuple(card));

    resize(parent->parent->width(), parent->parent->height());
}