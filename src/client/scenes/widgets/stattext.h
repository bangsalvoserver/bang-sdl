#ifndef __STATTEXT_H__
#define __STATTEXT_H__

#include "utils/sdl.h"

#include "defaults.h"

#include <string>

DECLARE_RESOURCE(arial_ttf)

namespace sdl {
    struct text_style {
        color text_color = rgb(sdl::default_text_rgb);
        resource_view text_font = GET_RESOURCE(arial_ttf);
        int text_ptsize = sdl::default_text_ptsize;
        int wrap_length = 0;
        color bg_color = rgba(sdl::default_text_bg_rgba);
        int bg_border_x = 5;
        int bg_border_y = 2;
    };

    class stattext {
    private:
        text_style m_style;
        font m_font;

        texture m_tex;

        rect m_rect;

        std::string m_value;

        int m_wrap_length = 0;

    public:
        stattext(const text_style &style = {})
            : m_style(style)
            , m_font(style.text_font, style.text_ptsize)
            , m_wrap_length(style.wrap_length) {}

        stattext(const std::string &label, const text_style &style = {})
            : stattext(style)
        {
            redraw(label);
        }

        void redraw(const std::string &label) {
            if (label != m_value) {
                m_value = label;
                m_tex = make_text_surface(label, m_font, m_wrap_length, m_style.text_color);
                m_rect = m_tex.get_rect();
            }
        }

        const std::string &get_value() const {
            return m_value;
        }

        void render(renderer &renderer, bool render_background = true) {
            if (m_tex) {
                if (render_background) {
                    renderer.set_draw_color(m_style.bg_color);
                    renderer.fill_rect(sdl::rect{
                        m_rect.x - m_style.bg_border_x, m_rect.y - m_style.bg_border_y,
                        m_rect.w + m_style.bg_border_x * 2, m_rect.h + m_style.bg_border_y * 2});
                }

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

        void set_rect(const rect &rect) noexcept {
            m_rect = rect;
        }

        const rect &get_rect() const noexcept {
            return m_rect;
        }

        void set_wrap_length(int length) noexcept {
            m_wrap_length = length;
        }

        int get_wrap_length() const noexcept {
            return m_wrap_length;
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