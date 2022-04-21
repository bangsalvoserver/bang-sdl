#ifndef __PLAY_VERIFY_H__
#define __PLAY_VERIFY_H__

#include <variant>
#include <vector>

#include "card_enums.h"
#include "holders.h"

namespace banggame {

    std::optional<game_error> check_player_filter(player *origin, target_player_filter filter, player *target);
    std::optional<game_error> check_card_filter(player *origin, target_card_filter filter, card *target);

    struct play_card_verify {
        player *origin;
        card *card_ptr;
        bool is_response;
        target_list targets;
        std::vector<card *> modifiers;

        void verify_modifiers();

        player *verify_equip_target();
        void verify_card_targets();

        opt_fmt_str check_prompt();
        opt_fmt_str check_prompt_equip(player *target);
        
        void play_modifiers() const;
        void do_play_card() const;

        void verify_and_play();
    };

}

#endif