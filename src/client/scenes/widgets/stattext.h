#ifndef __STATTEXT_H__
#define __STATTEXT_H__

#include "utils/sdl.h"

#include <string>

DECLARE_RESOURCE(arial_ttf)

namespace sdl {

    struct text_style {
        SDL_Color text_color;
        resource text_font;
        int text_ptsize;
    };

    static const text_style default_text_style {
        {0x0, 0x0, 0x0, 0xff},
        GET_RESOURCE(arial_ttf),
        18
    };

    class stattext {
    private:
        text_style m_style;
        font m_font;

        texture m_tex;

        SDL_Rect m_rect;

    public:
        stattext(const text_style &style = default_text_style)
            : m_style(style)
            , m_font(style.text_font, style.text_ptsize) {}

        stattext(const std::string &label, const text_style &style = default_text_style)
            : stattext(style)
        {
            redraw(label);
        }

        void redraw(const std::string &label) {
            m_tex = make_text_surface(label, m_font, m_style.text_color);
            m_rect = m_tex.get_rect();
        }

        void render(renderer &renderer) {
            if (m_tex) {
                m_tex.render(renderer, m_rect);
            }
        }

        void set_rect(const SDL_Rect &rect) {
            m_rect = rect;
        }

        const SDL_Rect &get_rect() const noexcept {
            return m_rect;
        }

        void set_point(const SDL_Point &pt) {
            m_rect.x = pt.x;
            m_rect.y = pt.y;
        }

        explicit operator bool() const {
            return bool(m_tex);
        }
    };
}

#endif