#ifndef __CARD_FWD_H__
#define __CARD_FWD_H__

#include "utils/enum_bitset.h"
#include "utils/nullable.h"
#include "utils/misc.h"

namespace banggame {
    
    struct card_view;
    struct player_view;

    using card_ptr = card_view *;
    using player_ptr = player_view *;

    using const_card_ptr = const card_view *;
    using const_player_ptr = const player_view *;

    using card_list = std::vector<card_ptr>;
    using player_list = std::vector<player_ptr>;

    using nullable_card = utils::nullable<card_ptr>;
    using nullable_player = utils::nullable<player_ptr>;

    enum class target_player_filter;
    enum class target_card_filter;
    enum class tag_type;
    
    enum class effect_flag;
    enum class game_flag;
    enum class player_flag;
    enum class discard_all_reason;

    using effect_flags = enums::bitset<effect_flag>;
    using game_flags = enums::bitset<game_flag>;
    using player_flags = enums::bitset<player_flag>;

}

#endif