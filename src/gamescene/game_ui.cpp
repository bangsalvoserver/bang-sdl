#include "game_ui.h"
#include "game.h"

#include <numeric>

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
    , m_leave_btn(_("BUTTON_EXIT"), [parent]{ parent->parent->add_message<banggame::client_message_type::lobby_leave>(); })
    , m_golobby_btn(_("BUTTON_TOLOBBY"), [parent]{ parent->parent->add_message<banggame::client_message_type::lobby_return>(); })
    , m_chat_btn(_("BUTTON_CHAT"), [parent]{ parent->parent->enable_chat(); }) {}

void game_ui::refresh_layout() {
    const auto win_rect = parent->parent->get_rect();

    m_game_log.set_rect(sdl::rect{20, win_rect.h - 450, 190, 400});

    int x = (win_rect.w - std::transform_reduce(m_special_btns.begin(), m_special_btns.end(), 100, std::plus(),
        [](const button_card_pair &pair) {
            return pair.first.get_rect().w + 10;
        })) / 2;
    m_confirm_btn.set_rect(sdl::rect{x, win_rect.h - 40, 100, 25});
    x += 110;

    for (auto &[btn, card] : m_special_btns) {
        sdl::rect rect = btn.get_rect();
        btn.set_rect(sdl::rect{x, win_rect.h - 40, rect.w, 25});
        x += rect.w + 10;
    }

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_golobby_btn.set_rect(sdl::rect{140, 20, 180, 25});

    m_chat_btn.set_rect(sdl::rect{win_rect.w - 120, win_rect.h - 40, 100, 25});

    if (m_message_box) {
        m_message_box->refresh_layout(win_rect);
    }
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
    m_golobby_btn.render(renderer);

    m_chat_btn.render(renderer);

    auto draw_status = [&](widgets::stattext &text) {
        if (text.get_value().empty()) return;

        sdl::rect rect = text.get_rect();
        rect.x = (parent->parent->width() - rect.w) / 2;
        rect.y = (parent->parent->height() - rect.h) - options.status_text_y_distance;
        text.set_rect(rect);
        text.render(renderer);
    };
    
    draw_status(m_status_text);

    if (m_message_box) {
        m_message_box->render(renderer);
    }
}

void game_ui::add_game_log(const std::string &message) {
    m_game_log.add_message(message);
}

void game_ui::add_special(card_view *card) {
    auto &btn = m_special_btns.emplace_back(std::piecewise_construct,
        std::make_tuple(_(intl::category::cards, card->name), [&target = parent->m_target, card]{
            if (target.is_card_clickable()) {
                target.on_click_card(pocket_type::specials, nullptr, card);
            }
        }), std::make_tuple(card)).first;

    btn.set_rect(sdl::rect{0, 0, std::max(100, btn.get_text_rect().w + 10), 25});

    refresh_layout();
}

void game_ui::remove_special(card_view *card) {
    auto it = std::ranges::find(m_special_btns, card, &decltype(m_special_btns)::value_type::second);
    if (it != m_special_btns.end()) {
        m_special_btns.erase(it);
        refresh_layout();
    }
}