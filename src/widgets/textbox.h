#ifndef __TEXTBOX_H__
#define __TEXTBOX_H__

#include "event_handler.h"
#include "stattext.h"

namespace widgets {

    struct textbox_style {
        text_style text;
        sdl::color background_color = default_textbox_background_color;
        sdl::color border_color = default_textbox_border_color;
        sdl::color selection_color = default_textbox_selection_color;
        int margin = 4;
        std::chrono::milliseconds cycle_duration{850};
    };

    class textbox : public event_handler {
    private:
        textbox_style m_style;

        sdl::font m_font;
        sdl::auto_texture m_tex;

        sdl::rect m_border_rect;

        std::string m_value;

        button_callback_fun on_enter;

        duration_type m_timer{0};
        int m_cursor_pos = 0;
        int m_cursor_len = 0;
        int m_hscroll = 0;

        bool m_mouse_down = false;

        void redraw() {
            m_tex = make_text_surface(m_value, m_font, 0, m_style.text.text_color);
        }

    protected:
        bool handle_event(const sdl::event &event);

        void on_gain_focus() override;
        void on_lose_focus() override;

    public:
        textbox(const textbox_style &style = {});

        void tick(duration_type time_elapsed);
        void render(sdl::renderer &renderer);

        const sdl::rect &get_rect() const {
            return m_border_rect;
        }

        void set_rect(const sdl::rect &rect) {
            m_border_rect = rect;
        }

        const std::string &get_value() const {
            return m_value;
        }

        void set_value(std::string value) {
            m_value = std::move(value);
            redraw();
        }

        void set_onenter(button_callback_fun &&fun) {
            on_enter = std::move(fun);
        }
    };

}

#endif