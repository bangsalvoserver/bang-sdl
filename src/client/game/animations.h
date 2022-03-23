#ifndef __ANIMATIONS_H__
#define __ANIMATIONS_H__

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
            : card(card), flips(flips) {}

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

    struct deck_flip_animation {
        card_pile_view *cards;
        bool flips;

        deck_flip_animation(card_pile_view *cards, bool flips)
            : cards(cards), flips(flips) {}

        void end() {
            for (card_view *card : *cards) {
                card_flip_animation(card, flips).end();
            }
        }

        void do_animation(float amt) {
            for (card_view *card : *cards) {
                card_flip_animation(card, flips).do_animation(amt);
            }
        }

        void render(sdl::renderer &renderer) {
            if (!cards->empty()) {
                card_flip_animation(cards->front(), flips).render(renderer);
            }
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

    struct pause_animation {
        card_view *card = nullptr;

        void render(sdl::renderer &renderer) {
            if (card) card->render(renderer, false);
        }

        void do_animation(float) {
            if (card) card->animating = true;
        }

        void end() {
            if (card) card->animating = false;
        }
    };
}

#endif