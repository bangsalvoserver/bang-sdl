#include "animations.h"

namespace banggame {

    using namespace sdl::point_math;

    float ease_in_out_pow(float exp, float x) {
        return x < 0.5f ? std::pow(2.f * x, exp) / 2.f : 1.f - std::pow(-2.f * x + 2.f, exp) / 2.f;
    }

    constexpr sdl::point lerp_point(sdl::point begin, sdl::point end, float amt) {
        return {
            int(std::lerp(float(begin.x), float(end.x), amt)),
            int(std::lerp(float(begin.y), float(end.y), amt))
        };
    }

    constexpr sdl::color lerp_color(sdl::color begin, sdl::color end, float amt) {
        return {
            static_cast<uint8_t>(std::lerp(float(begin.r), float(end.r), amt)),
            static_cast<uint8_t>(std::lerp(float(begin.g), float(end.g), amt)),
            static_cast<uint8_t>(std::lerp(float(begin.b), float(end.b), amt)),
            static_cast<uint8_t>(std::lerp(float(begin.a), float(end.a), amt))
        };
    }

    void player_move_animation::add_move_player(player_view *player, sdl::point end) {
        data.emplace_back(player, player->get_position(), end);
    }

    void player_move_animation::end() {
        for (auto &p : data) {
            p.player->set_position(p.end);
        }
    }

    void player_move_animation::do_animation_impl(float amt) {
        for (auto &p : data) {
            p.player->set_position(lerp_point(p.begin, p.end, amt));
        }
    }

    void card_move_animation::add_move_card(card_view *card) {
        if (std::ranges::find(data, card, &decltype(data)::value_type::first) == data.end()) {
            data.emplace_back(card, card->get_pos());
        }
    }

    void card_move_animation::end() {
        for (auto &[card, _] : data) {
            card->set_pos(card->pocket->get_pos() + card->pocket->get_offset(card));
            card->animating = false;
        }
    }

    void card_move_animation::do_animation_impl(float amt) {
        for (auto &[card, start] : data) {
            card->set_pos(lerp_point(start, card->pocket->get_pos() + card->pocket->get_offset(card), amt));
            card->animating = true;
        }
    }

    void card_move_animation::render(sdl::renderer &renderer) {
        for (auto &[card, _] : data) {
            card->render(renderer, render_flags::no_skip_animating);
        }
    }

    void card_flip_animation::end() {
        if (flips) {
            card->texture_front.reset();
            card->texture_front_scaled.reset();
            card->flip_amt = 0.f;
        } else {
            card->flip_amt = 1.f;
        }
        card->animating = false;
    }

    void card_flip_animation::do_animation_impl(float amt) {
        card->animating = true;
        card->flip_amt = flips ? 1.f - amt : amt;
    }

    void card_flip_animation::render(sdl::renderer &renderer) {
        card->render(renderer, render_flags::no_skip_animating);
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
            card->set_pos(lerp_point(start_pos, cards->get_pos(), amt));
            n -= diff;
        }
    }

    void deck_shuffle_animation::render(sdl::renderer &renderer) {
        for (card_view *card : *cards) {
            card->render(renderer, render_flags::no_skip_animating);
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
        card->render(renderer, render_flags::no_skip_animating);
    }

    card_flash_animation::card_flash_animation(card_view *card, sdl::color color_to)
        : card(card)
        , color_from(card->border_color.a
            ? card->border_color
            : sdl::color{color_to.r, color_to.g, color_to.b, 0})
        , color_to(color_to) {}

    void card_flash_animation::end() {
        card->animating = false;
        card->border_color = color_from;
    }

    void card_flash_animation::do_animation(float amt) {
        card->animating = true;
        card->border_color = lerp_color(color_to, color_from, amt);
    }

    void card_flash_animation::render(sdl::renderer &renderer) {
        card->render(renderer, render_flags::no_skip_animating);
    }

    void player_hp_animation::do_animation_impl(float amt) {
        player->set_hp_marker_position(std::lerp(float(hp_from), float(hp_to), amt));
    }

    void cube_move_animation::render(sdl::renderer &renderer) {
        for (auto &item : data) {
            item.cube->render(renderer, render_flags::no_skip_animating);
        }
    }

    void cube_move_animation::do_animation(float x) {
        const float off = options.move_cubes_offset;
        const float diff = off / data.size();
        const float m = 1.f / (1.f - off);
        
        float n = off;
        for (auto &item : data) {
            const float amt = ease_in_out_pow(options.easing_exponent, data.size() == 1 ? x : std::clamp(m * (x - n), 0.f, 1.f));

            item.cube->pos = lerp_point(item.start, item.pile->get_pos() + item.offset, amt);
            item.cube->animating = true;
            n -= diff;
        }
    }

    void cube_move_animation::end() {
        for (auto &item : data) {
            item.cube->pos = item.pile->get_pos() + item.offset;
            item.cube->animating = false;
        }
    }

    void pause_animation::render(sdl::renderer &renderer) {
        if (card) {
            card->render(renderer, render_flags::no_skip_animating);
        }
    }

    void pause_animation::do_animation(float) {
        if (card) {
            card->animating = true;
        }
    }

    void pause_animation::end() {
        if (card) {
            card->animating = false;
        }
    }

}