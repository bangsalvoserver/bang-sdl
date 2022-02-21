#include "profile_pic.h"
#include "../global_resources.h"

namespace widgets {

sdl::surface profile_pic::scale_profile_image(sdl::surface &&image) {
    sdl::rect rect = image.get_rect();
    if (rect.w > rect.h) {
        if (rect.w > size) {
            return sdl::scale_surface(image, rect.w / size);
        }
    } else {
        if (rect.h > size) {
            return sdl::scale_surface(image, rect.h / size);
        }
    }
    return std::move(image);
}

profile_pic::profile_pic() {
    set_texture(global_resources::get().icon_default_user);
}

void profile_pic::set_texture(const sdl::texture &tex) {
    if (tex) {
        m_texture = &tex;
    } else {
        m_texture = &global_resources::get().icon_default_user;
    }

    sdl::point pos = get_pos();
    m_rect = m_texture->get_rect();
    if (m_rect.w > m_rect.h) {
        sdl::scale_rect_width(m_rect, size);
    } else {
        sdl::scale_rect_height(m_rect, size);
    }
    set_pos(pos);
}

void profile_pic::set_pos(sdl::point pt) {
    m_rect.x = pt.x - m_rect.w / 2;
    m_rect.y = pt.y - m_rect.h / 2;
}

sdl::point profile_pic::get_pos() const {
    return sdl::point{m_rect.x + m_rect.w / 2, m_rect.y + m_rect.h / 2};
}

void profile_pic::render(sdl::renderer &renderer) {
    m_texture->render(renderer, m_rect);
}

bool profile_pic::handle_event(const sdl::event &event) {
    if (m_onclick && event.type == SDL_MOUSEBUTTONDOWN && sdl::point_in_rect(
        sdl::point{
            event.button.x,
            event.button.y},
        sdl::rect{
            m_rect.x + (m_rect.w - size) / 2,
            m_rect.y + (m_rect.h - size) / 2,
            size, size
        }))
    {
        m_onclick();
        return true;
    }
    return false;
}

}