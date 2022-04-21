#ifndef __BASE_EFFECTS_H__
#define __BASE_EFFECTS_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_play_card_action {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_max_usages {
        int max_usages;
        
        opt_error verify(card *origin_card, player *origin) const;
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_pass_turn {
        opt_error verify(card *origin_card, player *origin) const;
        opt_fmt_str on_prompt(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_resolve {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };
    
    struct effect_bang {
        void on_play(card *origin_card, player *origin, player *target, effect_flags flags = {});
    };

    struct effect_bangcard {
        void on_play(card *origin_card, player *origin, player *target, effect_flags flags = {});
    };

    struct effect_banglimit {
        opt_error verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_indians {
        void on_play(card *origin_card, player *origin, player *target, effect_flags flags = {});
    };

    struct effect_duel {
        void on_play(card *origin_card, player *origin, player *target, effect_flags flags = {});
    };
    
    struct effect_missed {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bangresponse {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };
    
    struct effect_barrel : effect_missed {
        void on_play(card *origin_card, player *target);
    };

    struct effect_beer {
        opt_fmt_str on_prompt(card *origin_card, player *origin) const {
            return on_prompt(origin_card, origin, origin);
        }
        opt_fmt_str on_prompt(card *origin_card, player *origin, player *target) const;
        
        void on_play(card *origin_card, player *origin) {
            on_play(origin_card, origin, origin);
        }
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_heal {
        int amount;
        effect_heal(int value) : amount(std::max(1, value)) {}

        opt_fmt_str on_prompt(card *origin_card, player *origin) const {
            return on_prompt(origin_card, origin, origin);
        }
        opt_fmt_str on_prompt(card *origin_card, player *origin, player *target) const;

        void on_play(card *origin_card, player *origin) {
            on_play(origin_card, origin, origin);
        }
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_heal_notfull : effect_heal {
        opt_error verify(card *origin_card, player *origin, player *target) const;
    };

    struct effect_saloon {
        opt_fmt_str on_prompt(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_deathsave {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_steal {
        opt_fmt_str on_prompt(card *origin_card, player *origin, card *target) const;
        void on_play(card *origin_card, player *origin, card *target, effect_flags flags = {});
        void on_resolve(card *origin_card, player *origin, card *target);
    };

    struct effect_destroy {
        void on_play(card *origin_card, player *origin, card *target, effect_flags flags = {});
        void on_resolve(card *origin_card, player *origin, card *target);
    };

    struct effect_choose_card {
        void on_play(card *origin_card, player *origin, card *target);
    };

    struct effect_draw {
        int ncards;
        effect_draw(int value) : ncards(std::max(1, value)) {}
        
        void on_play(card *origin_card, player *origin) {
            on_play(origin_card, origin, origin);
        }
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_draw_discard {
        opt_error verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_draw_to_discard {
        int ncards;
        effect_draw_to_discard(int value) : ncards(std::max(1, value)) {}

        void on_play(card *origin_card, player *origin);
    };

    struct effect_while_drawing : effect_empty {
        bool can_respond(card *origin_card, player *origin) const;
    };

    struct effect_drawing : effect_while_drawing {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_draw_one_less {
        void on_play(card *origin_card, player *target);
    };

    struct effect_generalstore {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_damage {
        opt_error verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };

}

#endif