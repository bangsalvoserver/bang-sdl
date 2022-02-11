#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include "player.h"

namespace banggame {

    struct card_move_animation {
        std::vector<std::pair<card_view*, sdl::point>> data;

        void add_move_card(card_view *card) {
            if (std::ranges::find(data, card, &decltype(data)::value_type::first) == data.end()) {
                data.emplace_back(card, card->get_pos());
            }
        }

        void end() {
            for (auto &[card, _] : data) {
                card->set_pos(card->pile->get_position_of(card));
                card->animating = false;
            }
        }

        void do_animation(float amt) {
            for (auto &[card, start] : data) {
                card->animating = true;
                sdl::point dest = card->pile->get_position_of(card);
                card->set_pos(sdl::point{
                    (int) std::lerp(start.x, dest.x, amt),
                    (int) std::lerp(start.y, dest.y, amt)
                });
            }
        }

        void render(sdl::renderer &renderer) {
            for (auto &[card, _] : data) {
                card->render(renderer, false);
            }
        }
    };

    struct card_flip_animation {
        card_view *card;
        bool flips;

        card_flip_animation(card_view *card, bool flips)
            : card(card), flips(flips) {
        }

        void end() {
            card->flip_amt = float(!flips);
            card->animating = false;
        }

        void do_animation(float amt) {
            card->animating = true;
            card->flip_amt = flips ? 1.f - amt : amt;
        }

        void render(sdl::renderer &renderer) {
            card->render(renderer, false);
        }
    };

    struct card_tap_animation {
        card_view *card;
        bool taps;

        void do_animation(float amt) {
            card->rotation = 90.f * (taps ? amt : 1.f - amt);
        }
    };

    struct player_hp_animation {
        player_view *player;

        int hp_from;
        int hp_to;

        void do_animation(float amt) {
            player->set_hp_marker_position(std::lerp(hp_from, hp_to, amt));
        }
    };

    struct cube_move_animation {
        cube_widget *cube;
        sdl::point start;
        sdl::point diff;

        cube_move_animation(cube_widget *cube, sdl::point diff)
            : cube(cube), diff(diff)
        {
            start = cube->pos;
            cube->animating = true;
        }

        void render(sdl::renderer &renderer) {
            cube->render(renderer, false);
        }
        
        void do_animation(float amt) {
            sdl::point dest{};
            if (cube->owner) dest = cube->owner->get_pos();
            cube->pos = sdl::point{
                (int) std::lerp(start.x, dest.x + diff.x, amt),
                (int) std::lerp(start.y, dest.y + diff.y, amt)
            };
        }

        void end() {
            sdl::point dest{};
            if (cube->owner) dest = cube->owner->get_pos();
            cube->pos = sdl::point{dest.x + diff.x, dest.y + diff.y};
            cube->animating = false;
        }
    };

    struct pause_animation {};

    class animation {
    private:
        int duration;
        int elapsed = 0;

        std::variant<card_move_animation, card_flip_animation, card_tap_animation, player_hp_animation, cube_move_animation, pause_animation> m_anim;

    public:
        template<typename ... Ts>
        animation(int duration, Ts && ... args)
            : duration(duration)
            , m_anim(std::forward<Ts>(args) ...) {}

        void tick() {
            ++elapsed;
            std::visit([this](auto &anim) {
                if constexpr (requires (float value) { anim.do_animation(value); }) {
                    anim.do_animation((float)elapsed / (float)duration);
                }
            }, m_anim);
        }

        void end() {
            elapsed = duration;
            std::visit([this](auto &anim) {
                if constexpr (requires { anim.end(); }) {
                    anim.end();
                }
            }, m_anim);
        }

        void render(sdl::renderer &renderer) {
            std::visit([&](auto &anim) {
                if constexpr (requires { anim.render(renderer); }) {
                    anim.render(renderer);
                }
            }, m_anim);
        }

        bool done() const {
            return elapsed >= duration;
        }
    };

}

#endif