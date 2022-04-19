#ifndef __PLAY_VERIFY_H__
#define __PLAY_VERIFY_H__

#include <variant>
#include <vector>

#include "card_enums.h"
#include "holders.h"

namespace banggame {

    class player;
    class card;

    struct target_none {};
    struct target_player {
        player *target;
    };
    struct target_card {
        player *target;
        card *target_card;
    };
    struct target_other_players {};
    struct target_cards_other_players {
        std::vector<card *> target_cards;
    };

    using play_card_target = std::variant<
        target_none,
        target_player,
        target_card,
        target_other_players,
        target_cards_other_players
    >;

    struct play_card_verify {
        player *origin;
        card *card_ptr;
        bool is_response;
        std::vector<play_card_target> targets;

        void verify_effect_player_target(target_player_filter filter, player *target);
        void verify_effect_card_target(const effect_holder &effect, player *target, card *target_card);

        void verify_equip_target();
        void verify_card_targets();

        void play_card_action() const;
        void log_played_card() const;
        void do_play_card() const;

        opt_fmt_str check_prompt();
        opt_fmt_str check_prompt_equip(player *target);
    };

    struct modifier_play_card_verify : play_card_verify {
        std::vector<card *> modifiers;

        void verify_modifiers();
        void play_modifiers() const;
    };

}

#endif