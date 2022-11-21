#include "profile_pic.h"
#include "../media_pak.h"

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

void profile_pic::set_texture(std::nullptr_t) {
    set_texture(media_pak::get().icon_default_user);
}

static sdl::surface generate_border_texture(sdl::texture_ref tex) {
    static constexpr size_t resolution = 4;
    static constexpr size_t circle_size = 5;

    static constexpr auto points = []{
        std::array<std::pair<float, float>, resolution * resolution> ret;
        auto it = ret.begin();
        for (size_t y=0; y<resolution; ++y) {
            for (size_t x=0; x<resolution; ++x) {
                *it++ = {
                    (0.5f + x) / resolution,
                    (0.5f + y) / resolution
                };
            }
        }
        return ret;
    }();

    static constexpr auto circle = []{
        constexpr float radius = circle_size * .5f;
        std::array<std::array<float, circle_size>, circle_size> ret{};
        for (size_t y=0; y<circle_size; ++y) {
            for (size_t x=0; x<circle_size; ++x) {
                for (auto [xx, yy] : points) {
                    float dx = radius - (x + xx);
                    float dy = radius - (y + yy);
                    float d = std::sqrt(dx*dx + dy*dy);
                    ret[y][x] += float(d <= radius) / points.size();
                }
            }
        }
        return ret;
    }();

    sdl::surface ret{profile_pic::size + circle_size, profile_pic::size + circle_size};
    SDL_LockSurface(ret.get());
    
    SDL_UnlockSurface(ret.get());
    return ret;
}

void profile_pic::set_texture(sdl::texture_ref tex) {
    if (tex) {
        m_texture = tex;
        m_border_texture = generate_border_texture(tex);

        sdl::point pos = get_pos();
        m_rect = m_texture.get_rect();
        if (m_rect.w > m_rect.h) {
            sdl::scale_rect_width(m_rect, size);
        } else {
            sdl::scale_rect_height(m_rect, size);
        }
        set_pos(pos);
    } else {
        set_texture(nullptr);
    }
}

void profile_pic::set_pos(sdl::point pt) {
    m_rect.x = pt.x - m_rect.w / 2;
    m_rect.y = pt.y - m_rect.h / 2;
}

sdl::point profile_pic::get_pos() const {
    return sdl::point{m_rect.x + m_rect.w / 2, m_rect.y + m_rect.h / 2};
}

void profile_pic::render(sdl::renderer &renderer) {
    if (m_border_color.a != 0) {
        auto border_rect = sdl::move_rect_center(m_border_texture.get_rect(), sdl::rect_center(m_rect));
        m_border_texture.render_colored(renderer, border_rect, m_border_color);
    }
    m_texture.render(renderer, m_rect);
}

bool profile_pic::handle_event(const sdl::event &event) {
    if (event.type == SDL_MOUSEBUTTONDOWN && sdl::point_in_rect(
        sdl::point{
            event.button.x,
            event.button.y},
        sdl::rect{
            m_rect.x + (m_rect.w - size) / 2,
            m_rect.y + (m_rect.h - size) / 2,
            size, size
        }))
    {
        switch (event.button.button) {
        case SDL_BUTTON_LEFT:
            if (m_onclick) {
                m_onclick();
                return true;
            }
            return false;
        case SDL_BUTTON_RIGHT:
            if (m_on_rightclick) {
                m_on_rightclick();
                return true;
            }
            return false;
        }
    }
    return false;
}

}