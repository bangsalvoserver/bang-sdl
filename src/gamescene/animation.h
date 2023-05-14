#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include "../widgets/defaults.h"

#include "animations.h"

#include <cassert>

namespace banggame {

    struct animation_vtable {
        void (*do_animation)(void *self, float amt);
        void (*end)(void *self);
        void (*render)(void *self, sdl::renderer &renderer);
        void (*move)(void *self, void *p) noexcept;
        void (*destruct)(void *self) noexcept;
    };

    template<typename T>
    concept animation_has_do_animation = requires (T value, float amt) {
        value.do_animation(amt);
    };

    template<typename T>
    concept animation_has_end = requires (T value) {
        value.end();
    };

    template<typename T>
    concept animation_has_render = requires (T value, sdl::renderer & renderer) {
        value.render(renderer);
    };

    template<typename T>
    inline void destruct(T& obj) {
        obj.~T();
    }

    template<typename T>
    const animation_vtable animation_vtable_for = {
        .do_animation = [](void *self, float amt) {
            if constexpr (animation_has_do_animation<T>) {
                static_cast<T *>(self)->do_animation(amt);
            }
        },

        .end = [](void *self) {
            if constexpr (animation_has_end<T>) {
                static_cast<T *>(self)->end();
            }
        },

        .render = [](void *self, sdl::renderer &renderer) {
            if constexpr (animation_has_render<T>) {
                static_cast<T *>(self)->render(renderer);
            }
        },

        .move = [](void *self, void *p) noexcept {
            static_assert(std::is_nothrow_move_constructible_v<T>);
            new (p) T(std::move(*static_cast<T *>(self)));
        },

        .destruct = [](void *self) noexcept {
            static_assert(std::is_nothrow_destructible_v<T>);
            destruct(*static_cast<T*>(self));
        }
    };

    class animation_object {
    private:
        std::aligned_storage_t<32> m_data;
        const animation_vtable *vtable;
    
    public:
        template<typename T>
        animation_object(std::in_place_type_t<T>, auto && ... args)
            : vtable(&animation_vtable_for<T>)
        {
            static_assert(sizeof(T) <= sizeof(m_data));
            new (&m_data) T(FWD(args) ... );
        }

        animation_object(const animation_object &other) = delete;
        animation_object(animation_object &&other) noexcept
            : vtable(other.vtable)
        {
            vtable->move(&other.m_data, &m_data);
        }

        animation_object &operator = (const animation_object &other) = delete;
        animation_object &operator = (animation_object &&other) noexcept {
            assert(this != &other);
            vtable->destruct(&m_data);
            vtable = other.vtable;
            vtable->move(&other.m_data, &m_data);
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


    class animation {
    private:
        anim_duration_type duration;
        anim_duration_type elapsed{0};
        animation_object m_value;

    public:
        template<typename T>
        animation(anim_duration_type duration, std::in_place_type_t<T> tag, auto && ... args)
            : duration(duration)
            , m_value(tag, FWD(args) ...) {}

        void tick(anim_duration_type time_elapsed) {
            elapsed += time_elapsed;
            m_value.do_animation(std::clamp(elapsed / duration, 0.f, 1.f));
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

        anim_duration_type extra_time() const {
            return elapsed - duration;
        }
    };

}

#endif