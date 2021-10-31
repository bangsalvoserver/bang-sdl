#ifndef __GAME_TIMER_H__
#define __GAME_TIMER_H__

#include "effects.h"

namespace banggame {

    struct player;

    struct timer_base : request_base {
        int duration = 100;
    };

    struct timer_lemonade_jim : timer_base {
        timer_lemonade_jim() {
            duration = 200;
        }

        std::vector<int> players;
    };

    struct timer_damaging : timer_base {
        int damage;
        bool is_bang;

        void on_finished();
    };

}

#endif