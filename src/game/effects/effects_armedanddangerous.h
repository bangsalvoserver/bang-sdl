#ifndef __EFFECTS_ARMEDANDDANGEROUS_H__
#define __EFFECTS_ARMEDANDDANGEROUS_H__

#include "card_effect.h"

namespace banggame {

    struct handler_draw_atend {
        void on_play(card *origin_card, player *origin, mth_target_list targets);
    };

    struct effect_select_cube : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_pay_cube : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_add_cube : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };
    
    struct effect_reload : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_rust : card_effect {
        void on_play(card *origin_card, player *origin, player *target, effect_flags flags = {});
        void on_resolve(card *origin_card, player *origin, player *target);
    };

    struct effect_bandolier : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_belltower : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_doublebarrel : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_thunderer : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_buntlinespecial : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bigfifty : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_flintlock : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_duck : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct handler_squaw {
        void verify(card *origin_card, player *origin, mth_target_list targets) const;
        void on_play(card *origin_card, player *origin, mth_target_list targets);
    };

    struct effect_move_bomb : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin, player *target);
    };
}

#endif