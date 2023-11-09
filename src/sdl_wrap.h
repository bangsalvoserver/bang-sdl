#ifndef __SDL_WRAPPER_H__
#define __SDL_WRAPPER_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_rotozoom.h>

#include <fmt/format.h>
#include <stdexcept>
#include <memory>

#include "utils/resource.h"

namespace sdl {

    using rect = SDL_Rect;
    using point = SDL_Point;
    using color = SDL_Color;
    using event = SDL_Event;

    inline namespace point_math {
        constexpr point operator + (const point &lhs, const point &rhs) {
            return {lhs.x + rhs.x, lhs.y + rhs.y};
        }

        constexpr point operator - (const point &lhs, const point &rhs) {
            return {lhs.x - rhs.x, lhs.y - rhs.y};
        }

        constexpr point operator - (const point &pt) {
            return {-pt.x, -pt.y};
        }

        constexpr point operator * (const point &pt, int scalar) {
            return {pt.x * scalar, pt.y * scalar};
        }
    }

    constexpr rect move_rect(const rect &r, const point &top_left) {
        return {top_left.x, top_left.y, r.w, r.h};
    }

    constexpr rect move_rect_center(const rect &r, const point &center) {
        return move_rect(r, center - point{r.w / 2, r.h / 2});
    }

    constexpr point rect_center(const rect &r) {
        return {r.x + r.w / 2, r.y + r.h / 2};
    }

    constexpr color rgba(uint32_t color) {
        return {
            static_cast<uint8_t>((color & 0xff000000) >> (8 * 3)),
            static_cast<uint8_t>((color & 0x00ff0000) >> (8 * 2)),
            static_cast<uint8_t>((color & 0x0000ff00) >> (8 * 1)),
            static_cast<uint8_t>((color & 0x000000ff) >> (8 * 0))
        };
    }

    constexpr color rgb(uint32_t color) {
        return {
            static_cast<uint8_t>((color & 0xff0000) >> (8 * 2)),
            static_cast<uint8_t>((color & 0x00ff00) >> (8 * 1)),
            static_cast<uint8_t>((color & 0x0000ff) >> (8 * 0)),
            0xff
        };
    }

    constexpr sdl::color lerp_color(sdl::color begin, sdl::color end, float amt) {
        return {
            static_cast<uint8_t>(std::lerp(float(begin.r), float(end.r), amt)),
            static_cast<uint8_t>(std::lerp(float(begin.g), float(end.g), amt)),
            static_cast<uint8_t>(std::lerp(float(begin.b), float(end.b), amt)),
            static_cast<uint8_t>(std::lerp(float(begin.a), float(end.a), amt))
        };
    }

    constexpr sdl::color lerp_color_alpha(sdl::color begin, sdl::color end) {
        float amt = end.a / 255.f;
        return {
            static_cast<uint8_t>(std::lerp(float(begin.r), float(end.r), amt)),
            static_cast<uint8_t>(std::lerp(float(begin.g), float(end.g), amt)),
            static_cast<uint8_t>(std::lerp(float(begin.b), float(end.b), amt)),
            begin.a
        };
    }

    constexpr color full_alpha(color col) {
        return {col.r, col.g, col.b, 0xff};
    };

    struct error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct initializer {
        explicit initializer(uint32_t flags) {
            if (SDL_Init(flags) != 0) {
                throw error(fmt::format("Could not init SDL: {}", SDL_GetError()));
            }
        }

        ~initializer() {
            SDL_Quit();
        }
    };

    struct ttf_initializer {
        ttf_initializer() {
            if (TTF_Init() != 0) {
                throw error(fmt::format("Could not init SDL_ttf: {}", TTF_GetError()));
            }
        }

        ~ttf_initializer() {
            TTF_Quit();
        }
    };

    struct img_initializer {
        explicit img_initializer(int flags) {
            if ((IMG_Init(flags) & flags) != flags) {
                throw error(fmt::format("Could not init SDL_image: {}", IMG_GetError()));
            }
        }

        ~img_initializer() {
            IMG_Quit();
        }
    };

    struct window_deleter {
        void operator()(SDL_Window *value) {
            SDL_DestroyWindow(value);
        }
    };

    class window : public std::unique_ptr<SDL_Window, window_deleter> {
        using base = std::unique_ptr<SDL_Window, window_deleter>;

    public:
        window(const char *title, int x, int y, int w, int h, uint32_t flags)
            : base(SDL_CreateWindow(title, x, y, w, h, flags)) {
            if (!*this) throw error(fmt::format("Could not create window: {}", SDL_GetError()));
        }
    };

    struct renderer_deleter {
        void operator()(SDL_Renderer *value) {
            SDL_DestroyRenderer(value);
        }
    };

    class renderer : public std::unique_ptr<SDL_Renderer, renderer_deleter> {
        using base = std::unique_ptr<SDL_Renderer, renderer_deleter>;

    public:
        renderer(window &w, int index, uint32_t flags)
            : base(SDL_CreateRenderer(w.get(), index, flags))
        {
            if (!*this) throw error(fmt::format("Could not create renderer: {}", SDL_GetError()));
        }

        void set_draw_color(const color &color) {
            SDL_SetRenderDrawColor(get(), color.r, color.g, color.b, color.a);
        }

        void render_clear() {
            SDL_RenderClear(get());
        }

        void draw_rect(const rect &rect) {
            SDL_RenderDrawRect(get(), &rect);
        }

        void fill_rect(const rect &rect) {
            SDL_RenderFillRect(get(), &rect);
        }
    };
    
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        constexpr uint32_t rmask = 0xff000000;
        constexpr uint32_t gmask = 0x00ff0000;
        constexpr uint32_t bmask = 0x0000ff00;
        constexpr uint32_t amask = 0x000000ff;
    #else
        constexpr uint32_t rmask = 0x000000ff;
        constexpr uint32_t gmask = 0x0000ff00;
        constexpr uint32_t bmask = 0x00ff0000;
        constexpr uint32_t amask = 0xff000000;
    #endif

    struct surface_deleter {
        void operator()(SDL_Surface *value) {
            SDL_FreeSurface(value);
        }
    };

    class surface : public std::unique_ptr<SDL_Surface, surface_deleter> {
        using base = std::unique_ptr<SDL_Surface, surface_deleter>;

    public:
        surface() = default;
        surface(SDL_Surface *value) noexcept : base(value) {}

        surface(int width, int height)
            : base(SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask)) {
            if (!*this) throw error(fmt::format("Could not create surface: {}", SDL_GetError()));
        }

        explicit surface(resource_view res)
            : base(IMG_Load_RW(SDL_RWFromConstMem(res.data, int(res.length)), 0)) {
            if (!*this) throw error(fmt::format("Could not load image: {}", IMG_GetError()));
        }

        rect get_rect() const {
            rect rect{};
            if (*this) {
                SDL_GetClipRect(get(), &rect);
            }
            return rect;
        }
    };

    struct texture_deleter {
        void operator()(SDL_Texture *value) {
            SDL_DestroyTexture(value);
        }
    };

    struct render_ex_options {
        color color_modifier = sdl::rgba(0xffffffff);
        double angle{};
    };

    class texture_ref {
    private:
        SDL_Texture *m_value;

    public:
        texture_ref() = default;
        texture_ref(SDL_Texture *value) : m_value(value) {}

        SDL_Texture *get() const {
            return m_value;
        }

        operator bool() const {
            return m_value != nullptr;
        }

        rect get_rect() const {
            rect ret{};
            if (*this) {
                SDL_QueryTexture(m_value, nullptr, nullptr, &ret.w, &ret.h);
            }
            return ret;
        }

        void render(renderer &renderer, const rect &rect) const {
            SDL_RenderCopy(renderer.get(), m_value, nullptr, &rect);
        }
        
        void render(renderer &renderer, const point &pt) const {
            render(renderer, move_rect(get_rect(), pt));
        }

        void render_ex(renderer &renderer, const rect &rect, const render_ex_options &options) const {
            SDL_SetTextureColorMod(m_value, options.color_modifier.r, options.color_modifier.g, options.color_modifier.b);
            SDL_SetTextureAlphaMod(m_value, options.color_modifier.a);
            SDL_RenderCopyEx(renderer.get(), m_value, nullptr, &rect, options.angle, nullptr, SDL_FLIP_NONE);
            SDL_SetTextureColorMod(m_value, 0xff, 0xff, 0xff);
            SDL_SetTextureAlphaMod(m_value, 0xff);
        }
        
        void render_colored(renderer &renderer, const rect &rect, const color &col) const {
            render_ex(renderer, rect, render_ex_options{ .color_modifier = col });
        }
    };

    class texture : public std::unique_ptr<SDL_Texture, texture_deleter> {
        using base = std::unique_ptr<SDL_Texture, texture_deleter>;

        void check() {
            if (!*this) throw error(fmt::format("Could not create texture: {}", SDL_GetError()));
        }

    public:
        texture() = default;

        texture(SDL_Texture *value) : base(value) { check(); }
        
        texture(renderer &renderer, const surface &surf) {
            if (surf) {
                reset(SDL_CreateTextureFromSurface(renderer.get(), surf.get()));
                check();
            }
        }

        texture(renderer &renderer, resource_view res)
            : base(IMG_LoadTexture_RW(renderer.get(), SDL_RWFromConstMem(res.data, int(res.length)), 0)) {
            if (!*this) throw error(fmt::format("Could not create texture: {}", IMG_GetError()));
        }

        rect get_rect() const {
            return texture_ref(*this).get_rect();
        }

        void render(renderer &renderer, const rect &rect) const {
            texture_ref(*this).render(renderer, rect);
        }

        void render(renderer &renderer, const point &pt) const {
            texture_ref(*this).render(renderer, pt);
        }

        void render_ex(renderer &renderer, const rect &rect, const render_ex_options &options) const {
            texture_ref(*this).render_ex(renderer, rect, options);
        }

        void render_colored(renderer &renderer, const rect &rect, const color &col) const {
            texture_ref(*this).render_colored(renderer, rect, col);
        }

        operator texture_ref() const {
            return get();
        }
    };

    class auto_texture {
        surface m_surface;
        texture m_texture;
    
    public:
        auto_texture() = default;
        auto_texture(surface &&surf) : m_surface(std::move(surf)) {}

        rect get_rect() const {
            if (m_texture) {
                return m_texture.get_rect();
            } else {
                return m_surface.get_rect();
            }
        }

        operator bool() const {
            return m_surface || m_texture;
        }

        const texture &get_texture(renderer &renderer) {
            if (!m_texture) {
                m_texture = texture(renderer, m_surface);
                m_surface.reset();
            }
            return m_texture;
        }

        void render(renderer &renderer, const rect &rect) {
            get_texture(renderer).render(renderer, rect);
        }

        void render(renderer &renderer, const point &pt) {
            get_texture(renderer).render(renderer, pt);
        }

        void render_ex(renderer &renderer, const rect &rect, const render_ex_options &options) {
            get_texture(renderer).render_ex(renderer, rect, options);
        }

        void render_colored(renderer &renderer, const rect &rect, const color &col) {
            get_texture(renderer).render_colored(renderer, rect, col);
        }
    };

    struct font_deleter {
        void operator()(TTF_Font *value) {
            TTF_CloseFont(value);
        }
    };

    class font : public std::unique_ptr<TTF_Font, font_deleter> {
        using base = std::unique_ptr<TTF_Font, font_deleter>;

    public:
        font(resource_view res, int ptsize)
            : base(TTF_OpenFontRW(SDL_RWFromConstMem(res.data, int(res.length)), 0, ptsize)) {
            if (!*this) throw error(fmt::format("Could not create font: {}", TTF_GetError()));
        }
    };

    inline surface make_text_surface(const std::string &label, const font &font, int width, color text_color = rgb(0x0)) {
        if (label.empty()) {
            return surface();
        }
        SDL_Surface *s = width > 0
            ? TTF_RenderUTF8_Blended_Wrapped(font.get(), label.c_str(), text_color, width)
            : TTF_RenderUTF8_Blended(font.get(), label.c_str(), text_color);
        if (!s) {
            throw error(fmt::format("Could not render text: {}", TTF_GetError()));
        }
        return s;
    }

    inline bool point_in_rect(const point &pt, const rect &rect) {
        return pt.x >= rect.x && pt.x < (rect.x + rect.w)
            && pt.y >= rect.y && pt.y < (rect.y + rect.h);
    };

    inline void scale_rect_width(rect &rect, int new_width) {
        rect.h = new_width * rect.h / rect.w;
        rect.w = new_width;
    }

    inline void scale_rect_height(rect &rect, int new_height) {
        rect.w = new_height * rect.w / rect.h;
        rect.h = new_height;
    }

    inline surface scale_surface(const surface &surf, int scale) {
        return shrinkSurface(surf.get(), scale, scale);
    }

}

#endif