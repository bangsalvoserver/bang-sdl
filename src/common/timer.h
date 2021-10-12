#ifndef __TIMER_H__
#define __TIMER_H__

#include "utils/enum_variant.h"

namespace banggame {

    struct player;
    
    struct timer_beer_args {
        player *origin;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, timer_type,
        (none)
        (beer, timer_beer_args)
    )

    struct game_timer : enums::enum_variant<timer_type> {
        using enums::enum_variant<timer_type>::enum_variant;

        int duration = 0;

        void tick() {
            if (duration && --duration == 0) {
                emplace<timer_type::none>();
            }
        }
    };

}

#endif