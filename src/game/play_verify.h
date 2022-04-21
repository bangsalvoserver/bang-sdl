#ifndef __PLAY_VERIFY_H__
#define __PLAY_VERIFY_H__

#include <variant>
#include <vector>

#include "card_enums.h"
#include "holders.h"

namespace banggame {

    opt_error check_player_filter(player *origin, target_player_filter filter, player *target);
    opt_error check_card_filter(player *origin, target_card_filter filter, card *target);

    struct play_card_verify {
        player *origin;
        card *card_ptr;
        bool is_response;
        target_list targets;
        std::vector<card *> modifiers;

        [[nodiscard]] opt_error verify_modifiers() const;
        [[nodiscard]] opt_error verify_equip_target() const;
        [[nodiscard]] opt_error verify_card_targets() const;

        opt_fmt_str check_prompt() const;
        opt_fmt_str check_prompt_equip() const;

        player *get_equip_target() const;
        
        void play_modifiers() const;
        void do_play_card() const;

        [[nodiscard]] opt_error verify_and_play();
    };

}

#endif