#ifndef __PROFILE_PIC_H__
#define __PROFILE_PIC_H__

#include "event_handler.h"

namespace widgets {

    class profile_pic : public event_handler {
    public:
        static constexpr int size = 50;
        static sdl::surface scale_profile_image(sdl::surface &&image);

    public:
        profile_pic() {
            set_texture(nullptr);
        }

        profile_pic(sdl::texture &&tex) {
            set_texture(std::move(tex));
        }

        profile_pic(sdl::texture_ref tex) {
            set_texture(tex);
        }

        void set_texture(std::nullptr_t);
        void set_texture(sdl::texture &&tex) {
            set_texture(m_owned_texture = std::move(tex));
        }

        void set_texture(sdl::texture_ref tex);
        
        sdl::texture_ref get_texture() const {
            return m_texture;
        }

        sdl::texture_ref get_owned_texture() const {
            return m_owned_texture;
        }

        void set_pos(sdl::point pt);
        sdl::point get_pos() const;

        void set_border_color(sdl::color color) {
            m_border_color = color;
        }
        
        void render(sdl::renderer &renderer);

        void set_onclick(button_callback_fun &&fun) {
            m_onclick = std::move(fun);
        }

        void set_on_rightclick(button_callback_fun &&fun) {
            m_on_rightclick = std::move(fun);
        }

    protected:
        bool handle_event(const sdl::event &event) override;

    private:
        sdl::texture m_owned_texture;
        sdl::texture_ref m_texture;
        sdl::texture m_border_texture;

        sdl::rect m_rect;

        sdl::color m_border_color{};

        button_callback_fun m_onclick;
        button_callback_fun m_on_rightclick;
    };

}

#endif