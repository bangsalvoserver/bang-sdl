#ifndef __GAME_TIMER_H__
#define __GAME_TIMER_H__

#include "effects.h"

namespace banggame {

    struct player;

    struct timer_base : request_base {
        int duration = 100;
    };

    struct timer_beer : timer_base {
        std::vector<int> players;
    };

    struct timer_damaging : timer_base {
        int damage;
        bool is_bang;

        void on_finished();
    };

    struct timer_flightable : timer_base {
        timer_flightable() {
            duration = 150;
        }

        std::function<void()> on_finished;

        void on_resolve();
    };

}

#endif