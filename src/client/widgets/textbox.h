#ifndef __TEXTBOX_H__
#define __TEXTBOX_H__

#include "event_handler.h"
#include "stattext.h"

namespace widgets {

    struct textbox_style {
        text_style text;
        sdl::color background_color = sdl::rgb(0xffffff);
        sdl::color border_color = sdl::rgb(0x0);
        int margin = 4;
    };

    class textbox : public event_handler {
    private:
        textbox_style m_style;

        stattext m_tex;

        sdl::rect m_border_rect;

        button_callback_fun on_enter;

        int m_ticks = 0;
        int m_cursor_pos = 0;
        int m_cursor_len = 0;
        bool m_mouse_down = false;

    protected:
        bool handle_event(const sdl::event &event);

        void on_gain_focus() override;
        void on_lose_focus() override;

    public:
        textbox(const textbox_style &style = {});

        void render(sdl::renderer &renderer);

        const sdl::rect &get_rect() const noexcept {
            return m_border_rect;
        }

        void set_rect(const sdl::rect &rect) noexcept {
            m_border_rect = rect;
        }

        const std::string &get_value() const noexcept {
            return m_tex.get_value();
        }

        void set_value(std::string value) {
            m_tex.set_value(std::move(value));
        }

        void set_onenter(button_callback_fun &&fun) {
            on_enter = std::move(fun);
        }
    };

}

#endif