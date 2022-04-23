#ifndef __GOLDRUSH_CHARACTERS_H__
#define __GOLDRUSH_CHARACTERS_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_don_bell : event_based_effect {
        void on_enable(card *target_card, player *origin);
    };

    struct effect_madam_yto : event_based_effect {
        void on_enable(card *target_card, player *origin);
    };

    struct effect_dutch_will : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct request_dutch_will : selection_picker {
        request_dutch_will(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct effect_josh_mccloud {
        void on_play(card *origin_card, player *origin);
    };
}

#endif