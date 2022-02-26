#ifndef __REQUESTS_ARMEDANDDANGEROUS_H__
#define __REQUESTS_ARMEDANDDANGEROUS_H__

#include "card_effect.h"

namespace banggame {

    struct request_add_cube : request_base {
        request_add_cube(card *origin_card, player *target, int ncubes = 1)
            : request_base(origin_card, nullptr, target)
            , ncubes(ncubes) {}

        int ncubes = 1;
        
        bool can_pick(card_pile_type pile, player *target, card *target_card) const;
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text(player *owner) const;
    };

    struct request_move_bomb : request_base {
        request_move_bomb(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        game_formatted_string status_text(player *owner) const;
    };

    struct request_rust : request_base {
        using request_base::request_base;

        void on_resolve();
        game_formatted_string status_text(player *owner) const;
    };

    struct timer_al_preacher : request_base, timer_request {
        using request_base::request_base;
        game_formatted_string status_text(player *owner) const;
    };

    struct timer_tumbleweed : request_base, timer_request {
        timer_tumbleweed(card *origin_card, player *target, card *drawn_card, card *drawing_card)
            : request_base(origin_card, nullptr, target)
            , target_card(drawing_card)
            , drawn_card(drawn_card) {}
        
        card *target_card;
        card *drawn_card;

        void on_finished();
        game_formatted_string status_text(player *owner) const;
    };
}

#endif