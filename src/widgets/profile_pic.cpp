#include "profile_pic.h"
#include "../media_pak.h"

#include <array>

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

inline sdl::color &pixel_color(void *pixels, size_t x, size_t y, int pitch) {
    return *(reinterpret_cast<sdl::color *>(static_cast<std::byte *>(pixels) + y * pitch) + x);
}

static sdl::texture generate_border_texture(sdl::renderer &renderer, sdl::texture_ref source) {
    static constexpr size_t resolution = 3;
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
        constexpr float coefficient = 6.f / (points.size() * circle_size * circle_size);

        std::array<std::array<float, circle_size>, circle_size> ret{};
        for (size_t y=0; y<circle_size; ++y) {
            for (size_t x=0; x<circle_size; ++x) {
                for (auto [xx, yy] : points) {
                    float dx = radius - (x + xx);
                    float dy = radius - (y + yy);
                    float d2 = dx*dx + dy*dy;
                    ret[y][x] += float(d2 <= radius*radius) * coefficient;
                }
            }
        }
        return ret;
    }();

    sdl::rect target_rect{0, 0, profile_pic::size + circle_size, profile_pic::size + circle_size};
    auto source_pixels = std::make_unique_for_overwrite<std::byte[]>(target_rect.w * target_rect.h * 4);

    {
        sdl::texture target = SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, target_rect.w, target_rect.h);
        SDL_SetRenderTarget(renderer.get(), target.get());
        source.render(renderer, sdl::move_rect_center(source.get_rect(), sdl::rect_center(target_rect)));
        SDL_RenderReadPixels(renderer.get(), &target_rect, SDL_PIXELFORMAT_RGBA32, source_pixels.get(), target_rect.w * 4);
        SDL_SetRenderTarget(renderer.get(), nullptr);
    }

    sdl::texture ret = SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, target_rect.w, target_rect.h);
    SDL_SetTextureBlendMode(ret.get(), SDL_BLENDMODE_BLEND);

    void *ret_pixels;
    int pitch;
    SDL_LockTexture(ret.get(), &target_rect, &ret_pixels, &pitch);

    for (size_t y=0; y<target_rect.h; ++y) {
        for (size_t x=0; x<target_rect.w; ++x) {
            float value = 0.f;
            for (size_t cy = 0; cy < circle_size; ++cy) {
                for (size_t cx = 0; cx < circle_size; ++cx) {
                    sdl::point pt{
                        int(x + cx - circle_size / 2),
                        int(y + cy - circle_size / 2)
                    };

                    if (sdl::point_in_rect(pt, target_rect)) {
                        value += pixel_color(source_pixels.get(), pt.x, pt.y, pitch).a * circle[cy][cx];
                    }
                }
            }
            pixel_color(ret_pixels, x, y, pitch) = {0xff, 0xff, 0xff,
                static_cast<uint8_t>(std::clamp<float>(value, 0, 0xff))};
        }
    }

    SDL_UnlockTexture(ret.get());
    return ret;
}

void profile_pic::set_texture(sdl::texture_ref tex) {
    if (tex) {
        m_texture = tex;
        m_border_texture.reset();

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
        if (!m_border_texture) {
            m_border_texture = generate_border_texture(renderer, m_texture);
        }
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