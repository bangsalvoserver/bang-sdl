#ifndef __CARD_EFFECT_H__
#define __CARD_EFFECT_H__

#include "../card_enums.h"
#include "../game_action.h"
#include "../format_str.h"

#include <ranges>

namespace banggame {

    struct player;
    struct card;

    struct card_effect {REFLECTABLE(
        (play_card_target_type) target,
        (target_player_filter) player_filter,
        (target_card_filter) card_filter,
        (int) effect_value
    )};

    struct effect_empty {
        void on_play(card *origin_card, player *origin) {}
    };

    struct event_based_effect {
        void on_unequip(card *target_card, player *target);
    };

    struct predraw_check_effect {
        void on_unequip(card *target_card, player *target);
    };

    struct request_base {
        request_base(card *origin_card, player *origin, player *target, effect_flags flags = {})
            : origin_card(origin_card), origin(origin), target(target), flags(flags) {}
        
        virtual ~request_base() {}

        card *origin_card;
        player *origin;
        player *target;
        effect_flags flags;

        virtual game_formatted_string status_text(player *owner) const = 0;

        virtual bool can_pick(card_pile_type pile, player *target, card *target_card) const {
            return false;
        }

        virtual void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct resolvable_request {
        virtual void on_resolve() = 0;
    };

    struct timer_request {
        timer_request(int duration = 200)
            : duration(duration) {}

        int duration;

        void tick();
        virtual void on_finished() {}
    };

    struct selection_picker : request_base {
        using request_base::request_base;

        bool can_pick(card_pile_type pile, player *target_player, card *target_card) const {
            return pile == card_pile_type::selection;
        }
    };

    struct scenario_effect {
        void on_unequip(card *target_card, player *target) {}
    };

    using mth_target_list = std::vector<std::pair<player *, card *>>;

}


#endif