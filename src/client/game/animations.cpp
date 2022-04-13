#include "animations.h"

namespace banggame {

    float ease_in_out_pow(float exp, float x) {
        return x < 0.5f ? std::pow(2.f * x, exp) / 2.f : 1.f - std::pow(-2.f * x + 2.f, exp) / 2.f;
    }

    void card_move_animation::add_move_card(card_view *card) {
        if (std::ranges::find(data, card, &decltype(data)::value_type::first) == data.end()) {
            data.emplace_back(card, card->get_pos());
        }
    }

    void card_move_animation::end() {
        for (auto &[card, _] : data) {
            card->set_pos(card->pocket->get_position_of(card));
            card->animating = false;
        }
    }

    void card_move_animation::do_animation_impl(float amt) {
        for (auto &[card, start] : data) {
            card->animating = true;
            sdl::point dest = card->pocket->get_position_of(card);
            card->set_pos(sdl::point{
                (int) std::lerp(start.x, dest.x, amt),
                (int) std::lerp(start.y, dest.y, amt)
            });
        }
    }

    void card_move_animation::render(sdl::renderer &renderer) {
        for (auto &[card, _] : data) {
            card->render(renderer, false);
        }
    }

    void card_flip_animation::end() {
        card->flip_amt = float(!flips);
        card->animating = false;
    }

    void card_flip_animation::do_animation_impl(float amt) {
        card->animating = true;
        card->flip_amt = flips ? 1.f - amt : amt;
    }

    void card_flip_animation::render(sdl::renderer &renderer) {
        card->render(renderer, false);
    }

    void deck_shuffle_animation::end() {
        for (card_view *card : *cards) {
            card->animating = false;
            card->flip_amt = 0.f;
            card->set_pos(cards->get_pos());
        }
    }

    void deck_shuffle_animation::do_animation(float x) {
        const float off = options.shuffle_deck_offset;
        const float diff = off / cards->size();
        const float m = 1.f / (1.f - off);
        
        float n = off;
        for (card_view *card : *cards) {
            const float amt = ease_in_out_pow(options.easing_exponent, std::clamp(m * (x - n), 0.f, 1.f));
            card->animating = true;
            card->flip_amt = 1.f - amt;
            card->set_pos(sdl::point{
                (int) std::lerp(start_pos.x, cards->get_pos().x, amt),
                (int) std::lerp(start_pos.y, cards->get_pos().y, amt)
            });
            n -= diff;
        }
    }

    void deck_shuffle_animation::render(sdl::renderer &renderer) {
        for (card_view *card : *cards) {
            card->render(renderer, false);
        }
    }

    void card_tap_animation::end() {
        card->animating = false;
        card->rotation = taps ? 90.f : 0.f;
    }

    void card_tap_animation::do_animation_impl(float amt) {
        card->animating = true;
        card->rotation = 90.f * (taps ? amt : 1.f - amt);
    }

    void card_tap_animation::render(sdl::renderer &renderer) {
        card->render(renderer, false);
    }

    void player_hp_animation::do_animation_impl(float amt) {
        player->set_hp_marker_position(std::lerp(hp_from, hp_to, amt));
    }

    void cube_move_animation::render(sdl::renderer &renderer) {
        cube->render(renderer, false);
    }

    void cube_move_animation::do_animation_impl(float amt) {
        sdl::point dest{};
        if (cube->owner) dest = cube->owner->get_pos();
        cube->animating = true;
        cube->pos = sdl::point{
            (int) std::lerp(start.x, dest.x + diff.x, amt),
            (int) std::lerp(start.y, dest.y + diff.y, amt)
        };
    }

    void cube_move_animation::end() {
        sdl::point dest{};
        if (cube->owner) dest = cube->owner->get_pos();
        cube->pos = sdl::point{dest.x + diff.x, dest.y + diff.y};
        cube->animating = false;
    }

    void pause_animation::render(sdl::renderer &renderer) {
        card->render(renderer, false);
    }

    void pause_animation::do_animation(float) {
        card->animating = true;
    }

    void pause_animation::end() {
        card->animating = false;
    }

}