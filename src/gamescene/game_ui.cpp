#include "game_ui.h"
#include "game.h"

#include <numeric>

#include "../manager.h"

#include "cards/filter_enums.h"

using namespace banggame;

game_ui::game_ui(game_scene *parent)
    : parent(parent)
    , m_game_log(widgets::text_list_style{
        .text = {
            .text_ptsize = widgets::chat_log_ptsize
        }
    })
    , m_leave_btn(_("BUTTON_EXIT"), [parent]{ parent->parent->add_message<banggame::client_message_type::lobby_leave>(); })
    , m_golobby_btn(_("BUTTON_TOLOBBY"), [parent]{ parent->parent->add_message<banggame::client_message_type::lobby_return>(); })
    , m_chat_btn(_("BUTTON_CHAT"), [parent]{ parent->parent->enable_chat(); }) {}

void game_ui::refresh_layout() {
    const auto win_rect = parent->parent->get_rect();

    // TODO make scrollable and togglable
    m_game_log.set_rect(sdl::rect{20, win_rect.h - 450, 190, 400});

    m_leave_btn.set_rect(sdl::rect{20, 20, 100, 25});
    m_golobby_btn.set_rect(sdl::rect{140, 20, 180, 25});

    m_chat_btn.set_rect(sdl::rect{win_rect.w - 120, win_rect.h - 40, 100, 25});

    if (m_message_box) {
        m_message_box->refresh_layout(win_rect);
    }
}

void game_ui::render(sdl::renderer &renderer) {
    m_game_log.render(renderer);
    
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

void game_ui::clear_game_logs() {
    m_game_log.clear();
}

void button_row_pocket::set_pos(const sdl::point &pos) {
    m_pos = pos;

    constexpr int button_offset = 10;

    int tot_width = rn::accumulate(m_buttons 
        | rv::transform([](const widgets::button &btn) {
            return btn.get_rect().w + button_offset;
        }), -button_offset);
    int x = pos.x - tot_width / 2;

    for (auto &btn : m_buttons) {
        sdl::rect rect = btn.get_rect();
        btn.set_rect(sdl::rect{x, pos.y, rect.w, 25});
        x += rect.w + button_offset;
    }
}

void button_row_pocket::render(sdl::renderer &renderer) {
    auto it = begin();
    for (auto &btn : m_buttons) {
        card_view *card = *it;

        if (card->has_tag(tag_type::confirm) && parent->m_target.can_confirm()) {
            btn.set_toggled_color(colors.game_ui_button_confirm);
        } else if (auto style = card->get_style()) {
            btn.set_toggled_color(button_toggle_color(*style));
        } else {
            btn.set_toggled_color({});
        }

        btn.render(renderer);
        ++it;
    }
}

void button_row_pocket::add_card(card_view *card) {
    pocket_view::add_card(card);

    auto &button = m_buttons.emplace_back(std::string{}, [&target = parent->m_target, card]{
        if (target.is_card_clickable()) {
            target.on_click_card(pocket_type::button_row, nullptr, card);
        }
    }, widgets::button_style {
        .down_color = colors.game_ui_button_down
    });
    if (card->known) {
        update_button(card, button);
    } else {
        button.set_rect({});
    }
}

std::list<widgets::button>::iterator button_row_pocket::find_button(card_view *card) {
    auto btn_it = m_buttons.begin();
    auto card_it = m_cards.begin();
    while (card_it != m_cards.end() && *card_it != card) {
        ++btn_it;
        ++card_it;
    }
    return btn_it;
}

void button_row_pocket::update_button(card_view *card, widgets::button &button) {
    button.set_label(_(intl::category::cards, card->name));
    button.set_rect(sdl::rect{0, 0, std::max(100, button.get_text_rect().w + 10), 25});

    set_pos(get_pos());
}

void button_row_pocket::update_card(card_view *card) {
    if (auto btn_it = find_button(card); btn_it != m_buttons.end()) {
        update_button(card, *btn_it);
    }
}

void button_row_pocket::erase_card(card_view *card) {
    if (auto btn_it = find_button(card); btn_it != m_buttons.end()) {
        m_buttons.erase(btn_it);
        set_pos(get_pos());
    }

    pocket_view::erase_card(card);
}

void button_row_pocket::clear() {
    pocket_view::clear();
    m_buttons.clear();
}