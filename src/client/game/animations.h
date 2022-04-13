#ifndef __ANIMATIONS_H__
#define __ANIMATIONS_H__

#include "player.h"
    
namespace banggame {

    float ease_in_out_pow(float exp, float x);

    template<typename T>
    struct easing_animation {
        void do_animation(float amt) {
            static_cast<T &>(*this).do_animation_impl(ease_in_out_pow(options.easing_exponent, amt));
        }
    };
    
    struct card_move_animation : easing_animation<card_move_animation> {
        std::vector<std::pair<card_view*, sdl::point>> data;

        card_move_animation() = default;

        void add_move_card(card_view *card);

        void end();
        void do_animation_impl(float amt);
        void render(sdl::renderer &renderer);
    };

    struct card_flip_animation : easing_animation<card_flip_animation> {
        card_view *card;
        bool flips;

        card_flip_animation(card_view *card, bool flips)
            : card(card), flips(flips) {}

        void end();
        void do_animation_impl(float amt);
        void render(sdl::renderer &renderer);
    };

    struct deck_shuffle_animation {
        pocket_view *cards;
        sdl::point start_pos;

        deck_shuffle_animation(pocket_view *cards, sdl::point start_pos)
            : cards(cards), start_pos(start_pos) {}

        void end();
        void do_animation(float x);
        void render(sdl::renderer &renderer);
    };

    struct card_tap_animation : easing_animation<card_tap_animation> {
        card_view *card;
        bool taps;

        card_tap_animation(card_view *card, bool taps)
            : card(card), taps(taps) {}

        void end();
        void do_animation_impl(float amt);
        void render(sdl::renderer &renderer);
    };

    struct player_hp_animation : easing_animation<player_hp_animation> {
        player_view *player;

        int hp_from;
        int hp_to;

        player_hp_animation(player_view *player, int hp_from)
            : player(player), hp_from(hp_from), hp_to(player->hp) {}

        void do_animation_impl(float amt);
    };

    struct cube_move_animation : easing_animation<cube_move_animation> {
        cube_widget *cube;
        sdl::point start;
        sdl::point diff;

        cube_move_animation(cube_widget *cube, sdl::point diff)
            : cube(cube), start(cube->pos), diff(diff) {}

        void end();
        void do_animation_impl(float amt);
        void render(sdl::renderer &renderer);
    };

    struct pause_animation {
        card_view *card;

        pause_animation(card_view *card) : card(card) {}

        void end();
        void do_animation(float);
        void render(sdl::renderer &renderer);
    };
}

#endif