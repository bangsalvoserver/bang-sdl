#include "game_ui.h"
#include "game.h"

#include "../manager.h"

using namespace banggame;
using namespace enums::flag_operators;

game_ui::game_ui(game_scene *parent)
    : parent(parent)
    , m_game_log(widgets::text_list_style{
        .text = {
            .text_ptsize = widgets::chat_log_ptsize
        }
    })
    , m_confirm_btn(_("GAME_CONFIRM"), std::bind(&target_finder::on_click_confirm, &parent->m_target))
    , m_leave_btn(_("BUTTON_EXIT"), std::bind(&game_manager::add_message<client_message_type::lobby_leave>, parent->parent))
    , m_restart_btn(_("GAME_RESTART"), std::bind(&game_manager::add_message<client_message_type::game_start>, parent->parent))
    , m_chat_btn(_("BUTTON_CHAT"), std::bind(&game_manager::enable_chat, parent->parent)) {}

void game_ui::resize(int width, int height) {
    m_game_log.set_rect(sdl::rect{20, height - 450, 190, 400});

    int x = (width - m_special_btns.size() * 110 - 100) / 2;
    m_confirm_btn.set_rect(sdl::rect{x, height - 40, 100, 25});

    for (auto &[btn, card] : m_special_btns) {
        x += 110;
        btn.set_rect(sdl::rect{x, height - 40, 100, 25});
    }

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_restart_btn.set_rect(sdl::rect{140, 20, 100, 25});

    m_chat_btn.set_rect(sdl::rect{width - 120, height - 40, 100, 25});
}

void game_ui::render(sdl::renderer &renderer) {
    m_game_log.render(renderer);

    m_confirm_btn.set_toggled(parent->m_target.can_confirm());
    m_confirm_btn.render(renderer);

    for (auto &[btn, card] : m_special_btns) {
        btn.set_toggled(parent->m_target.is_playing_card(card) || parent->m_target.can_respond_with(card));
        btn.render(renderer);
    }
    
    m_leave_btn.render(renderer);
    m_restart_btn.render(renderer);

    m_chat_btn.render(renderer);

    auto draw_status = [&](widgets::stattext &text) {
        if (text.get_value().empty()) return;

        sdl::rect rect = text.get_rect();
        rect.x = (parent->parent->width() - rect.w) / 2;
        rect.y = (parent->parent->height() - rect.h) - options::status_text_y_distance;
        text.set_rect(rect);
        text.render(renderer);
    };
    
    draw_status(m_status_text);
}

void game_ui::add_game_log(const std::string &message) {
    m_game_log.add_message(message);
}

void game_ui::add_special(card_view *card) {
    m_special_btns.emplace_back(std::piecewise_construct,
        std::make_tuple(_(card->name), [&target = parent->m_target, card]{
            if (target.is_card_clickable()) {
                target.on_click_scenario_card(card);
            }
        }), std::make_tuple(card));

    resize(parent->parent->width(), parent->parent->height());
}