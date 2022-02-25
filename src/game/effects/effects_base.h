#ifndef __EFFECTS_BASE_H__
#define __EFFECTS_BASE_H__

#include "card_effect.h"

namespace banggame {

    struct effect_play_card_action : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_max_usages : card_effect {
        void verify(card *origin_card, player *origin) const;
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_pass_turn : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_resolve : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };
    
    struct effect_bang : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_bangcard : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_banglimit : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_indians : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_duel : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };
    
    struct effect_missed : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bangresponse : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };
    
    struct effect_barrel : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *target);
    };

    struct effect_beer : card_effect {
        void on_play(card *origin_card, player *origin) {
            on_play(origin_card, origin, origin);
        }
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_heal : card_effect {
        void on_play(card *origin_card, player *origin) {
            on_play(origin_card, origin, origin);
        }
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_heal_notfull : effect_heal {
        void verify(card *origin_card, player *origin, player *target) const;
    };

    struct effect_deathsave : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_destroy : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_choose_card : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_draw : card_effect {
        void on_play(card *origin_card, player *origin) {
            on_play(origin_card, origin, origin);
        }
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_draw_discard : card_effect {
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_draw_rest : card_effect {
        void on_play(card *origin_card, player *target);
    };

    struct effect_draw_done : card_effect {
        void on_play(card *origin_card, player *target);
    };

    struct effect_draw_skip : card_effect {
        void verify(card *origin_card, player *target) const;
        void on_play(card *origin_card, player *target);
    };

    struct effect_drawing : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin) {}
    };

    struct effect_steal : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_generalstore : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_damage : card_effect {
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };

}

#endif