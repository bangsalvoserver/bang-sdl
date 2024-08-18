#ifndef __GAME_OPTIONS_H__
#define __GAME_OPTIONS_H__

#include "card_defs.h"

#include "utils/enum_bitset.h"

namespace banggame {
    
    using game_duration = std::chrono::milliseconds;
    using animation_duration = std::chrono::milliseconds;
    
    struct game_options {
        enums::bitset<expansion_type> expansions;
        bool enable_ghost_cards;
        bool character_choice;
        bool quick_discard_all;
        int scenario_deck_size;
        int num_bots;
        game_duration damage_timer;
        game_duration escape_timer;
        game_duration bot_play_timer;
        game_duration tumbleweed_timer;
        float duration_coefficient;
        unsigned int game_seed;
    };

}

#endif