#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include "card.h"

namespace banggame {

    struct card_animation_item {
        sdl::point start;
        card_pile_view *pile;
    };

    struct card_move_animation {
        std::map<card_view *, card_animation_item> data;

        bool add_move_card(card_view *c) {
            auto [it, inserted] = data.try_emplace(c, c->pos, c->pile);
            it->first->animating = true;
            if (!inserted) {
                it->second.pile = c->pile;
                return false;
            }
            return true;
        }

        void do_animation(float amt) {
            for (auto &[card, pos] : data) {
                sdl::point dest = pos.pile->get_position(card);
                card->pos.x = std::lerp(pos.start.x, dest.x, amt);
                card->pos.y = std::lerp(pos.start.y, dest.y, amt);
            }
        }

        void end() {
            for (auto &[card, pos] : data) {
                card->pos = pos.pile->get_position(card);
                card->animating = false;
            }
        }
    };

    struct card_flip_animation {
        card_widget *card;
        bool flips;

        void do_animation(float amt) {
            card->flip_amt = flips ? 1.f - amt : amt;
        }
    };

    struct card_tap_animation {
        card_widget *card;
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

    struct pause_animation {};

    class animation {
    private:
        int duration;
        int elapsed = 0;

        std::variant<card_move_animation, card_flip_animation, card_tap_animation, player_hp_animation, pause_animation> m_anim;

    public:
        animation(int duration, decltype(m_anim) &&anim)
            : duration(duration)
            , m_anim(std::move(anim)) {}

        void tick() {
            ++elapsed;
            std::visit([this](auto &anim) {
                if constexpr (requires (float value) { anim.do_animation(value); }) {
                    anim.do_animation((float)elapsed / (float)duration);
                }
            }, m_anim);
        }

        void end() {
            std::visit([](auto &anim) {
                if constexpr (requires { anim.end(); }) {
                    anim.end();
                }
            }, m_anim);
        }

        bool done() const {
            return elapsed >= duration;
        }
    };

}

#endif