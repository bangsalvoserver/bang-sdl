#ifndef __CHECKBOX_H__
#define __CHECKBOX_H__

#include "button.h"

namespace widgets {
    
    using toggle_callback_fun = std::function<void(bool)>;

    class checkbox : public event_handler {
    private:
        button_style m_style;

        stattext m_text;
        sdl::rect m_border_rect;
        sdl::rect m_checkbox_rect;
        
        bool m_locked = false;
        toggle_callback_fun m_ontoggle;
        
        enum button_state {
            state_up,
            state_hover,
            state_down
        } m_state = state_up;

        bool m_value = false;

    protected:
        bool handle_event(const sdl::event &event) override;

    public:
        checkbox(const std::string &label, const button_style &style = {});

        void render(sdl::renderer &renderer);

        void set_rect(const sdl::rect &rect);

        const sdl::rect &get_rect() const {
            return m_border_rect;
        }

        bool get_value() const {
            return m_value;
        }

        void set_value(bool value) {
            m_value = value;
        }

        void set_locked(bool value) {
            m_locked = value;
        }

        void set_ontoggle(toggle_callback_fun &&fun) {
            m_ontoggle = std::move(fun);
        }
    };
}

#endif