#ifndef __STATTEXT_H__
#define __STATTEXT_H__

#include "common/sdl.h"

#include <string>

DECLARE_RESOURCE(arial_ttf)

namespace sdl {

    struct text_style {
        color text_color;
        resource_view text_font;
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

        rect m_rect;

        std::string m_value;

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
            if (label != m_value) {
                m_value = label;
                m_tex = make_text_surface(label, m_font, m_style.text_color);
                m_rect = m_tex.get_rect();
            }
        }

        const std::string &get_value() const {
            return m_value;
        }

        void render(renderer &renderer) {
            if (m_tex) {
                m_tex.render(renderer, m_rect);
            }
        }

        void render_cropped(renderer &renderer, const sdl::rect &crop_rect) {
            if (m_tex) {
                m_rect.x = crop_rect.x;
                m_rect.y = crop_rect.y;
                if (m_rect.w > crop_rect.w) {
                    sdl::rect src_rect{m_rect.w - crop_rect.w, 0, crop_rect.w, m_rect.h};
                    sdl::rect dst_rect{crop_rect.x, m_rect.y, crop_rect.w, m_rect.h};
                    SDL_RenderCopy(renderer.get(), m_tex.get_texture(renderer), &src_rect, &dst_rect);
                    m_rect.x -= src_rect.x;
                } else {
                    m_tex.render(renderer, m_rect);
                }
            }
        }

        void set_rect(const rect &rect) {
            m_rect = rect;
        }

        const rect &get_rect() const noexcept {
            return m_rect;
        }

        void set_point(const point &pt) {
            m_rect.x = pt.x;
            m_rect.y = pt.y;
        }

        explicit operator bool() const {
            return bool(m_tex);
        }
    };
}

#endif