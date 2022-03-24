#ifndef __ANIMATIONS_H__
#define __ANIMATIONS_H__

#include "player.h"
    
namespace banggame {

    inline float ease_in_out_pow(float exp, float x) {
        return x < 0.5f ? std::pow(2.f * x, exp) / 2.f : 1.f - std::pow(-2.f * x + 2.f, exp) / 2.f;
    }

    template<typename T>
    struct easing_animation {
        void do_animation(float amt) {
            static_cast<T &>(*this).do_animation_impl(ease_in_out_pow(options.easing_exponent, amt));
        }
    };
    
    struct card_move_animation : easing_animation<card_move_animation> {
        std::vector<std::pair<card_view*, sdl::point>> data;

        card_move_animation() = default;

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

        void do_animation_impl(float amt) {
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

    struct card_flip_animation : easing_animation<card_flip_animation> {
        card_view *card;
        bool flips;

        card_flip_animation(card_view *card, bool flips)
            : card(card), flips(flips) {}

        void end() {
            card->flip_amt = float(!flips);
            card->animating = false;
        }

        void do_animation_impl(float amt) {
            card->animating = true;
            card->flip_amt = flips ? 1.f - amt : amt;
        }

        void render(sdl::renderer &renderer) {
            card->render(renderer, false);
        }
    };

    struct deck_shuffle_animation {
        card_pile_view *cards;
        sdl::point start_pos;

        deck_shuffle_animation(card_pile_view *cards, sdl::point start_pos)
            : cards(cards), start_pos(start_pos) {}

        void end() {
            for (card_view *card : *cards) {
                card->flip_amt = 0.f;
                card->animating = false;
            }
        }

        void do_animation(float x) {
            size_t i = 0;
            for (card_view *card : *cards) {
                const float n = cards->size();
                const float off = options.shuffle_deck_offset;
                const float y = (x - ((n - i) / n) * off) / (1 - off);
                const float amt = ease_in_out_pow(options.easing_exponent, std::clamp(y, 0.f, 1.f));
                card->animating = true;
                card->flip_amt = 1.f - amt;
                card->set_pos(sdl::point{
                    (int) std::lerp(start_pos.x, cards->get_pos().x, amt),
                    (int) std::lerp(start_pos.y, cards->get_pos().y, amt)
                });
                ++i;
            }
        }

        void render(sdl::renderer &renderer) {
            for (card_view *card : *cards) {
                card->render(renderer, false);
            }
        }
    };

    struct card_tap_animation : easing_animation<card_tap_animation> {
        card_view *card;
        bool taps;

        card_tap_animation(card_view *card, bool taps)
            : card(card), taps(taps) {}

        void do_animation_impl(float amt) {
            card->rotation = 90.f * (taps ? amt : 1.f - amt);
        }
    };

    struct player_hp_animation : easing_animation<player_hp_animation> {
        player_view *player;

        int hp_from;
        int hp_to;

        player_hp_animation(player_view *player, int hp_from)
            : player(player), hp_from(hp_from), hp_to(player->hp) {}

        void do_animation_impl(float amt) {
            player->set_hp_marker_position(std::lerp(hp_from, hp_to, amt));
        }
    };

    struct cube_move_animation : easing_animation<cube_move_animation> {
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
        
        void do_animation_impl(float amt) {
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

    struct pause_animation {
        card_view *card;

        pause_animation(card_view *card) : card(card) {}

        void render(sdl::renderer &renderer) {
            card->render(renderer, false);
        }

        void do_animation(float) {
            card->animating = true;
        }

        void end() {
            card->animating = false;
        }
    };
}

#endif