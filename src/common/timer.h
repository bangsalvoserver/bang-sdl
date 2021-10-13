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

}

#endif