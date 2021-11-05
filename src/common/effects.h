#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include "common/card_enums.h"
#include "game_action.h"

#include <ranges>

namespace banggame {

    struct player;

    struct card_effect {
        effect_flags flags = enums::flags_none<effect_flags>;
        target_type target = enums::flags_none<target_type>;
        int args = 0;
    };

    struct request_base {
        int origin_card_id;
        player *origin;
        player *target;
        effect_flags flags = enums::flags_none<effect_flags>;
    };

    template<card_pile_type ... Es> using pile_list = enums::enum_sequence<Es ...>;
    
    struct effect_bang : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_bangcard : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_aim : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_banglimit : card_effect {
        bool can_play(int origin_card_id, player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_indians : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_duel : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_bangresponse : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_bangresponse_onturn : effect_bangresponse {
        bool can_respond(player *origin) const;
    };
    
    struct effect_missed : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_missedcard : effect_missed {
        bool can_respond(player *origin) const;
    };

    struct effect_bangmissed : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };
    
    struct effect_barrel : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *target);
    };

    struct effect_heal : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_damage : card_effect {
        bool can_play(int origin_card_id, player *origin, player *target) const;
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_empty : card_effect {
        void on_play(int origin_card_id, player *origin) {}
    };

    struct effect_beer : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_deathsave : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_destroy : card_effect {
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

    struct effect_virtual_destroy : card_effect {
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

    struct effect_virtual_copy : card_effect {
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

    struct effect_virtual_clear : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_draw : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_draw_discard : card_effect {
        bool can_play(int origin_card_id, player *origin, player *target) const;
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_draw_rest : card_effect {
        void on_play(int origin_card_id, player *target);
    };

    struct effect_draw_done : card_effect {
        void on_play(int origin_card_id, player *target);
    };

    struct effect_draw_skip : card_effect {
        bool can_play(int origin_card_id, player *target) const;
        void on_play(int origin_card_id, player *target);
    };

    struct effect_steal : card_effect {
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

    struct effect_generalstore : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_bandidos : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_tornado : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_poker : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_saved : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_escape : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_rum : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_goldrush : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_bottle : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_pardner : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_shopchoice : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_pay_cube : card_effect {
        bool can_play(int origin_card_id, player *origin) const {
            return can_play(origin_card_id, origin, origin, origin_card_id);
        }

        bool can_play(int origin_card_id, player *origin, player *target, int card_id) const;

        void on_play(int origin_card_id, player *origin) {
            on_play(origin_card_id, origin, origin, origin_card_id);
        }

        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

    struct effect_add_cube : card_effect {
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };
    
    struct effect_reload : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_rust : card_effect {
        void on_play(int origin_card_id, player *origin, player *target);
    };

    struct effect_bandolier : card_effect {
        bool can_play(int origin_card_id, player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_belltower : card_effect {
        bool can_play(int origin_card_id, player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_doublebarrel : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_thunderer : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_buntlinespecial : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_bigfifty : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_flintlock : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_duck : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_squaw_destroy : card_effect {
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

    struct effect_squaw_steal : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

}

#endif