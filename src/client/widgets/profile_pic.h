#ifndef __PROFILE_PIC_H__
#define __PROFILE_PIC_H__

#include "event_handler.h"

namespace widgets {

    class profile_pic : public event_handler {
    public:
        static constexpr int size = 50;
        static sdl::surface scale_profile_image(sdl::surface &&image);

    public:
        profile_pic();

        profile_pic(const sdl::texture &tex) {
            set_texture(tex);
        }
        profile_pic(sdl::texture &&) = delete;

        void set_texture(const sdl::texture &tex);
        void set_texture(sdl::texture &&) = delete;

        void set_pos(sdl::point pt);
        sdl::point get_pos() const;

        void set_border_color(sdl::color color) noexcept {
            m_border_color = color;
        }
        
        void render(sdl::renderer &renderer);

        void set_onclick(button_callback_fun &&fun) {
            m_onclick = std::move(fun);
        }

    protected:
        bool handle_event(const sdl::event &event) override;

    private:
        const sdl::texture *m_texture = nullptr;

        sdl::rect m_rect;

        sdl::color m_border_color{};

        button_callback_fun m_onclick;
    };

}

#endif