#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include "animations.h"

namespace banggame {

    struct animation_vtable {
        void (*do_animation)(void *self, float amt);
        void (*end)(void *self);
        void (*render)(void *self, sdl::renderer &renderer);
        void (*copy)(const void *self, void *p);
        void (*move)(void *self, void *p) noexcept;
        void (*destruct)(void *self) noexcept;
    };

    template<typename T>
    const animation_vtable animation_vtable_for = {
        .do_animation = [](void *self, float amt) {
            if constexpr (requires (T value) { value.do_animation(amt); }) {
                static_cast<T *>(self)->do_animation(amt);
            }
        },

        .end = [](void *self) {
            if constexpr (requires (T value) { value.end(); }) {
                static_cast<T *>(self)->end();
            }
        },

        .render = [](void *self, sdl::renderer &renderer) {
            if constexpr (requires (T value) { value.render(renderer); }) {
                static_cast<T *>(self)->render(renderer);
            }
        },

        .copy = [](const void *self, void *p) {
            new (p) T(*static_cast<const T *>(self));
        },

        .move = [](void *self, void *p) noexcept {
            static_assert(std::is_nothrow_move_constructible_v<T>);
            new (p) T(std::move(*static_cast<T *>(self)));
        },

        .destruct = [](void *self) noexcept {
            static_assert(std::is_nothrow_destructible_v<T>);
            static_cast<T *>(self)->~T();
        }
    };

    class animation_object {
    private:
        std::aligned_storage_t<24> m_data;
        const animation_vtable *vtable;
    
    public:
        template<typename T, typename ... Ts>
        animation_object(std::in_place_type_t<T>, Ts && ... args)
            : vtable(&animation_vtable_for<T>)
        {
            static_assert(sizeof(T) <= sizeof(m_data));
            new (&m_data) T(std::forward<Ts>(args) ... );
        }

        animation_object(const animation_object &other)
            : vtable(other.vtable)
        {
            vtable->copy(&other.m_data, &m_data);
        }

        animation_object(animation_object &&other) noexcept
            : vtable(other.vtable)
        {
            vtable->move(&other.m_data, &m_data);
        }

        animation_object &operator = (const animation_object &other) {
            animation_object copy(other);
            std::swap(m_data, copy.m_data);
            std::swap(vtable, copy.vtable);
            return *this;
        }

        animation_object &operator = (animation_object &&other) noexcept {
            animation_object copy(std::move(other));
            std::swap(m_data, copy.m_data);
            std::swap(vtable, copy.vtable);
            return *this;
        }

        ~animation_object() {
            vtable->destruct(&m_data);
        }

        void do_animation(float amt) {
            vtable->do_animation(&m_data, amt);
        }

        void end() {
            vtable->end(&m_data);
        }

        void render(sdl::renderer &renderer) {
            vtable->render(&m_data, renderer);
        }
    };

    inline float ease_in_out_pow(float exp, float x) {
        return x < 0.5f ? std::pow(2.f * x, exp) / 2.f : 1.f - std::pow(-2.f * x + 2.f, exp) / 2.f;
    }


    class animation {
    private:
        int duration;
        int elapsed = 0;
        animation_object m_value;

    public:
        template<typename T, typename ... Ts>
        animation(int duration, std::in_place_type_t<T> tag, Ts && ... args)
            : duration(duration)
            , m_value(tag, std::forward<Ts>(args) ...) {}

        void tick() {
            ++elapsed;
            m_value.do_animation(ease_in_out_pow(options.easing_exponent, (float)elapsed / (float)duration));
        }

        void end() {
            m_value.end();
        }

        void render(sdl::renderer &renderer) {
            m_value.render(renderer);
        }

        bool done() const {
            return elapsed >= duration;
        }
    };

}

#endif