#include "game_message_box.h"

#include "game_ui.h"

#include <numeric>

namespace banggame {
    game_message_box::game_message_box(game_ui *parent, const std::string &message, button_init_list &&buttons) 
        : m_message(message)
    {
        for (auto &[label, fun] : buttons) {
            auto &btn = m_buttons.emplace_back(label, [parent, fun=std::move(fun)]{
                fun();
                parent->close_message_box();
            });
            btn.set_rect(sdl::rect{0, 0, std::max(70, btn.get_text_rect().w + 10), 25});
        }
    }

    void game_message_box::refresh_layout(const sdl::rect &win_rect) {
        sdl::rect message_rect = m_message.get_rect();
        message_rect.x = (win_rect.w - message_rect.w) / 2;
        message_rect.y = (win_rect.h - message_rect.h) / 2 - 25;
        m_message.set_rect(message_rect);

        constexpr int button_offset = 10;

        int tot_width = std::transform_reduce(m_buttons.begin(), m_buttons.end(), -button_offset, std::plus(),
            [](const widgets::button &btn) {
                return btn.get_rect().w + button_offset;
            });
        int x = (win_rect.w - tot_width) / 2;
        int y = message_rect.y + message_rect.h + 10;

        for (auto &btn : m_buttons) {
            sdl::rect rect = btn.get_rect();
            btn.set_rect(sdl::rect{x, y, rect.w, 25});
            x += rect.w + button_offset;
        }

        int bg_width = std::max({tot_width + 20, message_rect.w + 20, 170});
        m_bg_rect = sdl::rect{
            (win_rect.w - bg_width) / 2, message_rect.y - 10,
            bg_width, message_rect.h + 55
        };
    }

    void game_message_box::render(sdl::renderer &renderer) {
        renderer.set_draw_color(colors.status_text_background);
        renderer.fill_rect(m_bg_rect);

        m_message.render(renderer, false);
        for (auto &button : m_buttons) {
            button.render(renderer);
        }
    }
}