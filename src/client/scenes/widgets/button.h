#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <functional>

#include "utils/sdl.h"
#include "stattext.h"

namespace sdl {

    struct button_style : text_style {
        SDL_Color up_color;
        SDL_Color hover_color;
        SDL_Color down_color;
        SDL_Color border_color;
    };

    static const button_style default_button_style {
        default_text_style.text_color,
        default_text_style.text_font,
        default_text_style.text_ptsize,
        {0xee, 0xee, 0xee, 0xff},
        {0xdd, 0xdd, 0xff, 0xff},
        {0xcc, 0xcc, 0xff, 0xff},
        {0x0, 0x0, 0x0, 0xff},
    };

    using button_callback_fun = std::function<void()>;

    class button {
    private:
        button_style m_style;
        stattext m_text;

        SDL_Rect m_border_rect;
        SDL_Point m_text_pos;

        button_callback_fun m_onclick;

        enum button_state {
            state_up,
            state_hover,
            state_down
        } m_state = state_up;

    public:
        button(const std::string &label, button_callback_fun &&onclick = nullptr, const button_style &style = default_button_style);
        
        void render(renderer &renderer);
        void handle_event(const SDL_Event &event);

        void set_onclick(button_callback_fun &&onclick) {
            m_onclick = std::move(onclick);
        }

        void set_rect(const SDL_Rect &rect) {
            m_border_rect = rect;
            
            auto text_rect = m_text.get_rect();
            m_text_pos.x = rect.x + (rect.w - text_rect.w) / 2;
            m_text_pos.y = rect.y + (rect.h - text_rect.h) / 2;
        }

        const SDL_Rect &get_rect() const noexcept {
            return m_border_rect;
        }
    };

}

#endif