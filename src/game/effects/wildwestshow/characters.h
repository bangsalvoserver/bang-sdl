#ifndef __WILDWESTSHOW_CHARACTERS_H__
#define __WILDWESTSHOW_CHARACTERS_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_big_spencer {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_gary_looter : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_john_pain : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_teren_kill {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_youl_grinner : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct request_youl_grinner : request_base {
        request_youl_grinner(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct handler_flint_westwood {
        void on_play(card *origin_card, player *origin, const target_list &targets);
    };

    struct effect_greygory_deck : event_based_effect {
        void on_equip(card *target_card, player *target);
        void on_play(card *origin_card, player *origin);
    };

}

#endif