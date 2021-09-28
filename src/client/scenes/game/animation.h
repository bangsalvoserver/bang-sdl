#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include "card.h"

namespace banggame {

    struct card_animation_item {
        SDL_Point start;
        card_pile_view *pile;
    };

    struct card_move_animation {
        std::map<card_view *, card_animation_item> data;

        void add_move_card(card_view &c) {
            auto [it, inserted] = data.try_emplace(&c, c.pos, c.pile);
            if (!inserted) {
                it->second.pile = c.pile;
            }
        }

        void do_animation(float amt) {
            for (auto &[card, pos] : data) {
                SDL_Point dest = pos.pile->get_position(card->id);
                card->pos.x = std::lerp(pos.start.x, dest.x, amt);
                card->pos.y = std::lerp(pos.start.y, dest.y, amt);
            }
        }
    };

    struct card_flip_animation {
        card_view *card;
        bool flips;

        void do_animation(float amt) {
            card->flip_amt = flips ? 1.f - amt : amt;
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

        }
    };

    class animation {
    private:
        int duration;
        int elapsed = 0;

        std::variant<card_move_animation, card_flip_animation, card_tap_animation, player_hp_animation> m_anim;

    public:
        animation(int duration, decltype(m_anim) &&anim)
            : duration(duration)
            , m_anim(std::move(anim)) {}

        void tick() {
            ++elapsed;
            std::visit([this](auto &anim) {
                anim.do_animation((float)elapsed / (float)duration);
            }, m_anim);
        }

        bool done() const {
            return elapsed >= duration;
        }
    };

}

#endif