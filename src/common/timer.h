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
        timer_tumbleweed(card *origin_card, player *origin, card *drawn_card, card *target_card) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;

            timer_base::duration = 200;
            
            timer_tumbleweed::m_target_card = target_card;
            timer_tumbleweed::drawn_card = drawn_card;
        }
        
        card *m_target_card;
        card *drawn_card;

        void on_finished();
        void on_resolve() { on_finished(); }

        card *target_card() const { return m_target_card; }
    };

}

#endif