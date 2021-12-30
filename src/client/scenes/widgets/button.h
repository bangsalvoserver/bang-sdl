#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "event_handler.h"
#include "stattext.h"

namespace sdl {

    struct button_style {
        text_style text;
        color up_color = sdl::rgb(sdl::default_button_up_rgb);
        color hover_color = sdl::rgb(sdl::default_button_hover_rgb);
        color down_color = sdl::rgb(sdl::default_button_down_rgb);
        color border_color = sdl::rgb(sdl::default_button_border_rgb);
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
        button(const std::string &label, button_callback_fun &&onclick = nullptr, const button_style &style = {});
        
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