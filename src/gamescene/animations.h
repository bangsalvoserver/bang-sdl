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

    struct player_move_animation : easing_animation<player_move_animation> {
        struct animation_entry {
            player_view *player;
            sdl::point begin;
            sdl::point end;
        };
        std::vector<animation_entry> data;

        player_move_animation() = default;

        void add_move_player(player_view *player, sdl::point end);

        void end();
        void do_animation_impl(float amt);
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

    struct card_flash_animation {
        card_view *card;
        sdl::color color_from;
        sdl::color color_to;

        card_flash_animation(card_view *card, sdl::color color_to);

        void end();
        void do_animation(float x);
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

    struct cube_move_animation {
        struct cube_animation_item {
            cube_widget *cube;
            cube_pile_base *pile;
            sdl::point start;
            sdl::point offset;
        };

        std::vector<cube_animation_item> data;

        cube_move_animation() = default;

        void add_cube(cube_widget *cube, cube_pile_base *pile) {
            data.emplace_back(cube, pile, cube->pos, pile->get_offset(cube));
        }

        void end();
        void do_animation(float amt);
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