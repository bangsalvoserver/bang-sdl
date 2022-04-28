#include "game_message_box.h"

#include "game_ui.h"

namespace banggame {
    game_message_box::game_message_box(
            const std::string &message,
            std::function<void()> &&on_click_yes,
            std::function<void()> &&on_click_no)
        : m_message(message)
        , m_yes_btn(_("BUTTON_YES"), std::move(on_click_yes))
        , m_no_btn(_("BUTTON_NO"), std::move(on_click_no))
    {}

    void game_message_box::refresh_layout(const sdl::rect &win_rect) {
        sdl::rect message_rect = m_message.get_rect();
        message_rect.x = (win_rect.w - message_rect.w) / 2;
        message_rect.y = (win_rect.h - message_rect.h) / 2 - 25;
        m_message.set_rect(message_rect);

        m_yes_btn.set_rect(sdl::rect{
            win_rect.w / 2 - 75,
            message_rect.y + message_rect.h + 10,
            70,
            25
        });
        m_no_btn.set_rect(sdl::rect{
            win_rect.w / 2 + 5,
            message_rect.y + message_rect.h + 10,
            70,
            25
        });

        m_bg_rect = sdl::rect{
            std::min(message_rect.x - 10, win_rect.w / 2 - 85),
            message_rect.y - 10,
            std::max(message_rect.w + 20, 170),
            message_rect.h + 55
        };
    }

    void game_message_box::render(sdl::renderer &renderer) {
        renderer.set_draw_color(options.status_text_background);
        renderer.fill_rect(m_bg_rect);

        m_message.render(renderer, false);
        m_yes_btn.render(renderer);
        m_no_btn.render(renderer);
    }
}