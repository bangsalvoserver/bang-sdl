#ifndef __SDL_WRAPPER_H__
#define __SDL_WRAPPER_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_rotozoom.h>

#include <fmt/core.h>

#include <stdexcept>
#include <memory>

#include "resource.h"

namespace sdl {

    using rect = SDL_Rect;
    using point = SDL_Point;
    using color = SDL_Color;
    using event = SDL_Event;

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
            : base(IMG_Load_RW(SDL_RWFromConstMem(res.data, res.length), 0)) {
            if (!*this) throw error(fmt::format("Could not load image: {}", IMG_GetError()));
        }

        rect get_rect() const {
            rect rect;
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

    class texture {
    public:
        texture() = default;
        
        texture(surface &&surf) noexcept : m_surface(std::move(surf)) {}

        void reset() {
            m_surface.reset();
            m_texture.reset();
        }

        rect get_rect() const {
            return m_surface.get_rect();
        }

        const surface &get_surface() const { return m_surface; }

        SDL_Texture *get_texture(sdl::renderer &renderer) const {
            if (!m_texture) {
                m_texture.reset(SDL_CreateTextureFromSurface(renderer.get(), m_surface.get()));
            }
            return m_texture.get();
        }

        void render(sdl::renderer &renderer, const rect &rect) const {
            SDL_RenderCopy(renderer.get(), get_texture(renderer), nullptr, &rect);
        }

        void render(sdl::renderer &renderer, const point &pt) const {
            rect rect = get_rect();
            rect.x = pt.x;
            rect.y = pt.y;
            render(renderer, rect);
        }

        void render_colored(sdl::renderer &renderer, const rect &rect, const color &col) const {
            SDL_SetTextureColorMod(m_texture.get(), col.r, col.g, col.b);
            SDL_SetTextureAlphaMod(m_texture.get(), col.a);
            render(renderer, rect);
            SDL_SetTextureColorMod(m_texture.get(), 0xff, 0xff, 0xff);
            SDL_SetTextureAlphaMod(m_texture.get(), 0xff);
        }

        explicit operator bool() const {
            return bool(m_surface);
        }
    
    private:
        surface m_surface;
        mutable std::unique_ptr<SDL_Texture, texture_deleter> m_texture;
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
            : base(TTF_OpenFontRW(SDL_RWFromConstMem(res.data, res.length), 0, ptsize)) {
            if (!*this) throw error(fmt::format("Could not create font: {}", TTF_GetError()));
        }
    };

    inline surface make_text_surface(const std::string &label, const sdl::font &font, int width, color text_color = rgb(0x0)) {
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
        return pt.x >= rect.x && pt.x <= (rect.x + rect.w)
            && pt.y >= rect.y && pt.y <= (rect.y + rect.h);
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