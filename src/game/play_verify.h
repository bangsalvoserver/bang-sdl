#ifndef __PLAY_VERIFY_H__
#define __PLAY_VERIFY_H__

#include <variant>
#include <vector>

#include "card_enums.h"
#include "holders.h"

namespace banggame {

    struct play_card_verify {
        player *origin;
        card *card_ptr;
        bool is_response;
        target_list targets;

        void verify_effect_player_target(target_player_filter filter, player *target);
        void verify_effect_card_target(const effect_holder &effect, card *target);

        player *verify_equip_target();
        void verify_card_targets();

        void play_card_action() const;
        void log_played_card() const;
        void do_play_card() const;

        opt_fmt_str check_prompt();
        opt_fmt_str check_prompt_equip(player *target);
    };

    struct modifier_play_card_verify : play_card_verify {
        std::vector<card *> modifiers;

        void verify_card_targets();
        void verify_modifiers();
        void play_modifiers() const;
    };

}

#endif