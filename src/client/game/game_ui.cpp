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
    , m_confirm_btn(_("GAME_CONFIRM"), [parent]{ parent->m_target.on_click_confirm(); })
    , m_leave_btn(_("BUTTON_EXIT"), [parent]{ parent->parent->add_message<client_message_type::lobby_leave>(); })
    , m_restart_btn(_("GAME_RESTART"), [parent]{ parent->parent->add_message<client_message_type::game_start>(); })
    , m_chat_btn(_("BUTTON_CHAT"), [parent]{ parent->parent->enable_chat(); }) {}

void game_ui::resize(int width, int height) {
    m_game_log.set_rect(sdl::rect{20, height - 450, 190, 400});

    int x = (width - std::transform_reduce(m_special_btns.begin(), m_special_btns.end(), 100, std::plus(),
        [](const button_card_pair &pair) {
            return pair.first.get_rect().w + 10;
        })) / 2;
    m_confirm_btn.set_rect(sdl::rect{x, height - 40, 100, 25});
    x += 110;

    for (auto &[btn, card] : m_special_btns) {
        sdl::rect rect = btn.get_rect();
        btn.set_rect(sdl::rect{x, height - 40, rect.w, 25});
        x += rect.w + 10;
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
    auto &btn = m_special_btns.emplace_back(std::piecewise_construct,
        std::make_tuple(_(card->name), [&target = parent->m_target, card]{
            if (target.is_card_clickable()) {
                target.on_click_scenario_card(card);
            }
        }), std::make_tuple(card)).first;

    btn.set_rect(sdl::rect{0, 0, std::max(100, btn.get_text_rect().w + 10), 25});

    resize(parent->parent->width(), parent->parent->height());
}

void game_ui::remove_special(card_view *card) {
    auto it = std::ranges::find(m_special_btns, card, &decltype(m_special_btns)::value_type::second);
    if (it != m_special_btns.end()) {
        m_special_btns.erase(it);
        resize(parent->parent->width(), parent->parent->height());
    }
}