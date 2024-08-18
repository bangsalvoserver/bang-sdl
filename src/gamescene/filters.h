#ifndef __GAME_FILTERS_H__
#define __GAME_FILTERS_H__

#include "net/card_fwd.h"

namespace banggame::filters {

    bool is_player_alive(const_player_ptr origin);
    
    bool check_player_filter(const_card_ptr origin_card, const_player_ptr origin, enums::bitset<target_player_filter> filter, const_player_ptr target, const effect_context &ctx = {});

    bool check_card_filter(const_card_ptr origin_card, const_player_ptr origin, enums::bitset<target_card_filter> filter, const_card_ptr target, const effect_context &ctx = {});
}

#endif