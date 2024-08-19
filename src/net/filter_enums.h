#ifndef __CARDS_FILTER_ENUMS_H__
#define __CARDS_FILTER_ENUMS_H__

#include "utils/enums.h"

namespace banggame {

    enum class target_player_filter {
        any,
        dead,
        alive,
        self,
        notself,
        notsheriff,
        notorigin,
        range_1,
        range_2,
        reachable,
        target_set,
        not_empty_hand,
        not_empty_table,
        not_empty_cubes,
    };

    enum class target_card_filter {
        target_set,
        selection,
        table,
        hand,
        blue,
        black,
        train,
        blue_or_train,
        hearts,
        diamonds,
        clubs,
        spades,
        origin_card_suit,
        two_to_nine,
        ten_to_ace,
        bang,
        bangcard,
        not_bangcard,
        missed,
        missedcard,
        not_missedcard,
        beer,
        bronco,
        cube_slot,
        can_target_self,
        catbalou_panic,
    };
    
    enum class tag_type {
        preselect,
        button_color,
        ghost_card,
        resolve,
        pass_turn,
        pick,
        skip_logs,
        no_auto_discard,
        bangcard,
        missed,
        missedcard,
        play_as_bang,
        banglimit,
        bangmod,
        beer,
        indians,
        catbalou_panic,
        drawing,
        weapon,
        horse,
        card_choice,
        last_scenario_card,
        buy_cost,
        traincost,
        max_hp,
        initial_cards,
        bronco,
    };
}

#endif