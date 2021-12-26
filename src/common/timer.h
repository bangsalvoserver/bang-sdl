#ifndef __GAME_TIMER_H__
#define __GAME_TIMER_H__

#include "effects.h"

namespace banggame {

    struct player;

    struct timer_base : request_base {
        timer_base(card *origin_card, player *origin, player *target, int duration = 200)
            : request_base(origin_card, origin, target)
            , duration(duration) {}

        int duration;
    };

    struct timer_lemonade_jim : timer_base {
        using timer_base::timer_base;
        game_formatted_string status_text() const;
    };

    struct timer_al_preacher : timer_base {
        using timer_base::timer_base;
        game_formatted_string status_text() const;
    };

    struct timer_damaging : timer_base {
        timer_damaging(card *origin_card, player *source, player *target, int damage, bool is_bang)
            : timer_base(origin_card, target, nullptr, 140)
            , damage(damage)
            , is_bang(is_bang)
            , source(source) {}
        
        int damage;
        bool is_bang;
        player *source = nullptr;

        std::function<void()> cleanup_function;

        void on_finished();
        void cleanup();
        game_formatted_string status_text() const;
    };

    struct timer_tumbleweed : timer_base {
        timer_tumbleweed(card *origin_card, player *target, card *drawn_card, card *drawing_card)
            : timer_base(origin_card, nullptr, target)
            , target_card(drawing_card)
            , drawn_card(drawn_card) {}
        
        card *target_card;
        card *drawn_card;

        void on_finished();
        void on_resolve() { on_finished(); }
        game_formatted_string status_text() const;
    };

}

#endif