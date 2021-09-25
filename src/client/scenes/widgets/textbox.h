#ifndef __TEXTBOX_H__
#define __TEXTBOX_H__

#include "utils/sdl.h"
#include "stattext.h"

#include <string>

namespace sdl {

    struct textbox_style : text_style {
        SDL_Color background_color;
        SDL_Color border_color;
        int margin;
    };

    static const textbox_style default_textbox_style {
        default_text_style.text_color,
        default_text_style.text_font,
        default_text_style.text_ptsize,
        {0xff, 0xff, 0xff, 0xff},
        {0x0, 0x0, 0x0, 0xff},
        4
    };

    class textbox {
    private:
        textbox_style m_style;

        std::string m_value;
        stattext m_tex;

        SDL_Rect m_border_rect;

        bool m_active = false;

        void update_texture() {
            m_tex.redraw(m_value);
        }

    public:
        textbox(const textbox_style &style = default_textbox_style);

        void render(renderer &renderer);
        void handle_event(const SDL_Event &event);

        const SDL_Rect &get_rect() const noexcept {
            return m_border_rect;
        }

        void set_rect(const SDL_Rect &rect) noexcept {
            m_border_rect = rect;
        }

        const std::string &get_value() const noexcept {
            return m_value;
        }

        void set_value(const std::string &value) {
            m_value = value;
            update_texture();
        }
    };

}

#endif