#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "event_handler.h"
#include "stattext.h"

namespace sdl {

    struct button_style : text_style {
        color up_color;
        color hover_color;
        color down_color;
        color border_color;
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

    class button : public event_handler {
    private:
        button_style m_style;
        stattext m_text;

        sdl::rect m_border_rect;
        point m_text_pos;

        button_callback_fun m_onclick;

        enum button_state {
            state_up,
            state_hover,
            state_down
        } m_state = state_up;

        bool toggled = false;
    
    protected:
        bool handle_event(const event &event) override;
        
    public:
        button(const std::string &label, button_callback_fun &&onclick = nullptr, const button_style &style = default_button_style);
        
        void render(renderer &renderer);

        void set_onclick(button_callback_fun &&onclick) {
            m_onclick = std::move(onclick);
        }

        void set_rect(const sdl::rect &rect) {
            m_border_rect = rect;
            
            auto text_rect = m_text.get_rect();
            m_text_pos.x = rect.x + (rect.w - text_rect.w) / 2;
            m_text_pos.y = rect.y + (rect.h - text_rect.h) / 2;
        }

        const sdl::rect &get_rect() const noexcept {
            return m_border_rect;
        }

        void set_toggled(bool down) {
            toggled = down;
        }
    };

}

#endif