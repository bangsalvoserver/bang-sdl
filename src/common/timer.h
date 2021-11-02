#ifndef __GAME_TIMER_H__
#define __GAME_TIMER_H__

#include "effects.h"

namespace banggame {

    struct player;

    struct timer_base : request_base {
        int duration;
    };

    struct timer_lemonade_jim : timer_base {
        timer_lemonade_jim() {
            duration = 200;
        }

        std::vector<int> players;
    };

    struct timer_damaging : timer_base {
        timer_damaging() {
            duration = 100;
        }
        
        int damage;
        bool is_bang;

        void on_finished();
    };

    struct timer_bush : timer_base {
        timer_bush() {
            duration = 150;
        }
        
        card_suit_type suit;
        card_value_type value;

        void on_finished();
    };

}

#endif