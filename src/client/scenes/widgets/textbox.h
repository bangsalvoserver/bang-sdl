#ifndef __TEXTBOX_H__
#define __TEXTBOX_H__

#include "event_handler.h"
#include "stattext.h"

namespace sdl {

    struct textbox_style : text_style {
        color background_color;
        color border_color;
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

    class textbox : public event_handler {
    private:
        textbox_style m_style;

        std::string m_value;
        stattext m_tex;

        sdl::rect m_border_rect;

        button_callback_fun on_enter;

        int m_ticks;

        void update_texture() {
            m_tex.redraw(m_value);
        }

    protected:
        bool handle_event(const event &event);

        void on_gain_focus() override;
        void on_lose_focus() override;

    public:
        textbox(const textbox_style &style = default_textbox_style);

        void render(renderer &renderer);

        const sdl::rect &get_rect() const noexcept {
            return m_border_rect;
        }

        void set_rect(const sdl::rect &rect) noexcept {
            m_border_rect = rect;
        }

        const std::string &get_value() const noexcept {
            return m_value;
        }

        void set_value(const std::string &value) {
            m_value = value;
            update_texture();
        }

        void set_onenter(button_callback_fun &&fun) {
            on_enter = std::move(fun);
        }
    };

}

#endif