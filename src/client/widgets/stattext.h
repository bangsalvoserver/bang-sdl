#ifndef __STATTEXT_H__
#define __STATTEXT_H__

#include "utils/sdl.h"

#include "defaults.h"
#include "../media_pak.h"

#include <string>

namespace widgets {
    struct text_style {
        sdl::color text_color = sdl::rgb(widgets::default_text_rgb);
        resource media_pak::* text_font = &media_pak::font_arial;
        int text_ptsize = widgets::default_text_ptsize;
        int wrap_length = 0;
        sdl::color bg_color = sdl::rgba(widgets::default_text_bg_rgba);
        int bg_border_x = 5;
        int bg_border_y = 2;
    };

    class stattext {
    private:
        text_style m_style;
        sdl::font m_font;

        sdl::texture m_tex;

        sdl::rect m_rect;

        std::string m_value;

        int m_wrap_length = 0;

        void redraw() {
            m_tex = make_text_surface(m_value, m_font, m_wrap_length, m_style.text_color);
            m_rect = m_tex.get_rect();
        }

    public:
        stattext(const text_style &style = {})
            : m_style(style)
            , m_font(media_pak::get().*(style.text_font), style.text_ptsize)
            , m_wrap_length(style.wrap_length) {}

        stattext(std::string label, const text_style &style = {})
            : stattext(style)
        {
            set_value(std::move(label));
        }

        void set_value(std::string label) {
            if (label != m_value) {
                m_value = std::move(label);
                redraw();
            }
        }

        const std::string &get_value() const {
            return m_value;
        }

        void render(sdl::renderer &renderer, bool render_background = true) {
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

        void set_rect(const sdl::rect &rect) {
            m_rect = rect;
        }

        const sdl::rect &get_rect() const {
            return m_rect;
        }

        void set_wrap_length(int length) {
            m_wrap_length = length;
        }

        int get_wrap_length() const {
            return m_wrap_length;
        }

        void set_point(const sdl::point &pt) {
            m_rect.x = pt.x;
            m_rect.y = pt.y;
        }

        explicit operator bool() const {
            return bool(m_tex);
        }
    };
}

#endif