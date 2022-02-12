#ifndef __SDL_WRAPPER_H__
#define __SDL_WRAPPER_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_rotozoom.h>

#include <stdexcept>

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
                throw error(std::string("Could not init SDL: ") + SDL_GetError());
            }
        }

        ~initializer() {
            SDL_Quit();
        }
    };

    struct ttf_initializer {
        ttf_initializer() {
            if (TTF_Init() != 0) {
                throw error(std::string("Could not init SDL_ttf: ") + TTF_GetError());
            }
        }

        ~ttf_initializer() {
            TTF_Quit();
        }
    };

    struct img_initializer {
        explicit img_initializer(int flags) {
            if ((IMG_Init(flags) & flags) != flags) {
                throw error(std::string("Could not init SDL_image") + IMG_GetError());
            }
        }

        ~img_initializer() {
            IMG_Quit();
        }
    };

    class window {
    public:
        window(const char *title, int x, int y, int w, int h, uint32_t flags) {
            m_value = SDL_CreateWindow(title, x, y, w, h, flags);
            
            if (!m_value) {
                throw error(std::string("Could not create window: ") + SDL_GetError());
            }
        }

        window(const window &other) = delete;
        window(window &&other) noexcept {
            std::swap(m_value, other.m_value);
        }

        window &operator = (const window &other) = delete;
        window &operator = (window &&other) noexcept {
            std::swap(m_value, other.m_value);
            return *this;
        }

        ~window() {
            if (m_value) {
                SDL_DestroyWindow(m_value);
                m_value = nullptr;
            }
        }

        SDL_Window *get() const { return m_value; }

    private:
        SDL_Window *m_value = nullptr;
    };

    class renderer {
    public:
        renderer(window &w, int index, uint32_t flags) {
            m_value = SDL_CreateRenderer(w.get(), index, flags);
            if (!m_value) {
                throw error(std::string("Could not create renderer: ") + SDL_GetError());
            }
        }

        renderer(const renderer &other) = delete;
        renderer(renderer &&other) noexcept {
            std::swap(m_value, other.m_value);
        }

        renderer &operator = (const renderer &other) = delete;
        renderer &operator = (renderer &&other) noexcept {
            std::swap(m_value, other.m_value);
            return *this;
        }

        ~renderer() {
            if (m_value) {
                SDL_DestroyRenderer(m_value);
                m_value = nullptr;
            }
        }

        SDL_Renderer *get() const { return m_value; }

        void set_draw_color(const color &color) {
            SDL_SetRenderDrawColor(m_value, color.r, color.g, color.b, color.a);
        }

        void render_clear() {
            SDL_RenderClear(m_value);
        }

        void draw_rect(const rect &rect) {
            SDL_RenderDrawRect(m_value, &rect);
        }

        void fill_rect(const rect &rect) {
            SDL_RenderFillRect(m_value, &rect);
        }

    private:
        SDL_Renderer *m_value = nullptr;
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

    class surface {
    public:
        surface() = default;

        surface(int width, int height) {
            m_value = SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask);
            
            if (!m_value) {
                throw error(std::string("Could not create surface: ") + SDL_GetError());
            }
        }

        surface(SDL_Surface *value) noexcept : m_value(value) {}

        explicit surface(resource_view res) {
            m_value = IMG_Load_RW(SDL_RWFromConstMem(res.data, res.length), 0);
            if (!m_value) {
                throw error(std::string("Could not load image: ") + IMG_GetError());
            }
        }

        surface(const surface &other) = delete;
        surface(surface &&other) noexcept {
            std::swap(m_value, other.m_value);
        }

        surface &operator = (const surface &other) = delete;
        surface &operator = (surface &&other) noexcept {
            std::swap(m_value, other.m_value);
            return *this;
        }

        ~surface() {
            clear();
        }

        void clear() {
            if (m_value) {
                SDL_FreeSurface(m_value);
                m_value = nullptr;
            }
        }

        rect get_rect() const {
            rect rect;
            if (m_value) {
                SDL_GetClipRect(m_value, &rect);
            }
            return rect;
        }

        SDL_Surface *get() const noexcept { return m_value; }

        explicit operator bool() const noexcept{
            return m_value != nullptr;
        }

    private:
        SDL_Surface *m_value = nullptr;
    };

    class texture {
    public:
        texture() = default;
        
        texture(surface &&surf) : m_surface(std::move(surf)) {}
        
        texture(const texture &other) = delete;
        texture(texture &&other) noexcept
            : m_surface(std::move(other.m_surface)) {
            std::swap(m_texture, other.m_texture);
        }

        texture &operator = (const texture &other) = delete;
        texture &operator = (texture &&other) noexcept {
            std::swap(m_surface, other.m_surface);
            std::swap(m_texture, other.m_texture);
            return *this;
        }

        ~texture() {
            clear();
        }

        void clear() {
            if (m_texture) {
                SDL_DestroyTexture(m_texture);
                m_texture = nullptr;
            }
            if (m_surface) {
                m_surface.clear();
            }
        }

        rect get_rect() const {
            return m_surface.get_rect();
        }

        const surface &get_surface() const { return m_surface; }

        SDL_Texture *get_texture(sdl::renderer &renderer) const {
            if (!m_texture) {
                m_texture = SDL_CreateTextureFromSurface(renderer.get(), m_surface.get());
            }
            return m_texture;
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
            SDL_SetTextureColorMod(m_texture, col.r, col.g, col.b);
            render(renderer, rect);
            SDL_SetTextureColorMod(m_texture, 0xff, 0xff, 0xff);
        }

        explicit operator bool() const noexcept{
            return bool(m_surface);
        }
    
    private:
        surface m_surface;
        mutable SDL_Texture *m_texture = nullptr;
    };

    class font {
    public:
        font(resource_view res, int ptsize) {
            m_value = TTF_OpenFontRW(SDL_RWFromConstMem(res.data, res.length), 0, ptsize);
            if (!m_value) {
                throw error(std::string("Could not create font: ") + TTF_GetError());
            }
        }

        font(const font &other) = delete;
        font(font &&other) noexcept {
            std::swap(m_value, other.m_value);
        }

        font &operator = (const font &other) = delete;
        font &operator = (font &&other) noexcept {
            std::swap(m_value, other.m_value);
            return *this;
        }

        ~font() {
            if (m_value) {
                TTF_CloseFont(m_value);
                m_value = nullptr;
            }
        }

        TTF_Font *get() const noexcept { return m_value; }

    private:
        TTF_Font *m_value = nullptr;
    };

    inline surface make_text_surface(const std::string &label, const sdl::font &font, int width, color text_color = rgb(0x0)) {
        if (label.empty()) {
            return surface();
        }
        SDL_Surface *s = width > 0
            ? TTF_RenderUTF8_Blended_Wrapped(font.get(), label.c_str(), text_color, width)
            : TTF_RenderUTF8_Blended(font.get(), label.c_str(), text_color);
        if (!s) {
            throw error(std::string("Could not render text: ") + TTF_GetError());
        }
        return s;
    }

    inline bool point_in_rect(const point &pt, const rect &rect) {
        return pt.x >= rect.x && pt.x <= (rect.x + rect.w)
            && pt.y >= rect.y && pt.y <= (rect.y + rect.h);
    };

    inline void scale_rect(rect &rect, int new_width) {
        rect.h = new_width * rect.h / rect.w;
        rect.w = new_width;
    }

    inline surface scale_surface(const surface &surf, int scale) {
        return shrinkSurface(surf.get(), scale, scale);
    }

}

#endif