#ifndef __TIMER_H__
#define __TIMER_H__

#include "utils/enum_variant.h"

namespace banggame {

    struct player;
    
    struct timer_beer_args {
        std::vector<int> players;
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
                enums::visit([](auto tag, auto & ... obj) {
                    if constexpr (sizeof...(obj) > 0 && (requires { obj.on_finished(); } && ...)) {
                        (obj.on_finished(), ...);
                    }
                }, *this);
                emplace<timer_type::none>();
            }
        }
    };

}

#endif