#include "animation.h"

#include <cmath>

#include "options.h"

namespace banggame {

    inline float ease_in_out_pow(float exp, float x) {
        return x < 0.5f ? std::pow(2.f * x, exp) / 2.f : 1.f - std::pow(-2.f * x + 2.f, exp) / 2.f;
    }

    void animation::tick() {
        ++elapsed;
        std::visit([this](auto &anim) {
            if constexpr (requires (float value) { anim.do_animation(value); }) {
                anim.do_animation(ease_in_out_pow(options.easing_exponent, (float)elapsed / (float)duration));
            }
        }, m_anim);
    }

    void animation::end() {
        elapsed = duration;
        std::visit([this](auto &anim) {
            if constexpr (requires { anim.end(); }) {
                anim.end();
            }
        }, m_anim);
    }

    void animation::render(sdl::renderer &renderer) {
        std::visit([&](auto &anim) {
            if constexpr (requires { anim.render(renderer); }) {
                anim.render(renderer);
            }
        }, m_anim);
    }

}