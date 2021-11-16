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

        std::vector<player *> players;
    };

    struct timer_al_preacher : timer_base {
        timer_al_preacher() {
            duration = 200;
        }

        std::vector<player *> players;
    };

    struct timer_damaging : timer_base {
        timer_damaging() {
            duration = 100;
        }
        
        int damage;
        bool is_bang;

        std::function<void()> cleanup_function;

        void on_finished();
        void cleanup();
    };

    struct timer_tumbleweed : timer_base {
        timer_tumbleweed() {
            duration = 150;
        }
        
        card *drawn_card;

        void on_finished();
    };

}

#endif