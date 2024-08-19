#ifndef __CARDS_GAME_ENUMS_H__
#define __CARDS_GAME_ENUMS_H__

#include "utils/enums.h"

namespace banggame {
    
    enum class effect_flag {
        is_bang,
        is_missed,
        play_as_bang,
        escapable,
        single_target,
        multi_target,
        skip_target_logs,
    };
    
    enum class game_flag {
        game_over,
        invert_rotation,
        disable_equipping,
        phase_one_draw_discard,
        phase_one_override,
        disable_player_distances,
        treat_any_as_bang,
        hands_shown,
        free_for_all,
    };

    enum class player_flag {
        dead,
        ghost_1,
        ghost_2,
        temp_ghost,
        extra_turn,
        treat_missed_as_bang,
        treat_any_as_bang,
        role_revealed,
        skip_turn,
        removed,
        winner,
    };
    
    enum class discard_all_reason {
        death,
        sheriff_killed_deputy,
        disable_temp_ghost,
        discard_ghost,
    };
}

#endif