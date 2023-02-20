#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "event_handler.h"
#include "stattext.h"

namespace widgets {

    struct button_style {
        text_style text { .bg_color = sdl::rgba(0) };
        sdl::color up_color = default_button_up_color;
        sdl::color hover_color = default_button_hover_color;
        sdl::color down_color = default_button_down_color;
        sdl::color border_color = default_button_border_color;
        sdl::color toggled_color{};
    };

    class button : public event_handler {
    private:
        button_style m_style;
        stattext m_text;

        sdl::rect m_border_rect;
        sdl::point m_text_pos;

        button_callback_fun m_onclick;

        enum button_state {
            state_up,
            state_hover,
            state_down
        } m_state = state_up;
    
    protected:
        bool handle_event(const sdl::event &event) override;
        
    public:
        button(const std::string &label, button_callback_fun &&onclick = nullptr, const button_style &style = {});
        
        void render(sdl::renderer &renderer);

        void set_label(const std::string &label);

        void set_onclick(button_callback_fun &&onclick) {
            m_onclick = std::move(onclick);
        }

        void set_rect(const sdl::rect &rect) {
            m_border_rect = rect;
            
            auto text_rect = m_text.get_rect();
            m_text_pos.x = rect.x + (rect.w - text_rect.w) / 2;
            m_text_pos.y = rect.y + (rect.h - text_rect.h) / 2;
        }

        const sdl::rect &get_rect() const {
            return m_border_rect;
        }

        const sdl::rect &get_text_rect() const {
            return m_text.get_rect();
        }

        void set_toggled_color(sdl::color color) {
            m_style.toggled_color = color;
        }
    };

}

#endif