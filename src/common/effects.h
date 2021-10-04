#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include "common/card_enums.h"

namespace banggame {

    struct player;

    struct card_effect {
        target_type target = enums::flags_none<target_type>;
        int maxdistance = 0;
    };

    struct request_base {
        player *origin;
        player *target;
    };
    
    struct effect_bang : card_effect {
        void on_play(player *origin, player *target);
    };

    struct effect_bangcard : card_effect {
        void on_play(player *origin, player *target);
    };

    struct effect_banglimit : card_effect {
        bool can_play(player *target) const;
        void on_play(player *origin);
    };

    struct effect_indians : card_effect {
        void on_play(player *origin, player *target);
    };

    struct effect_duel : card_effect {
        void on_play(player *origin, player *target);
    };

    struct effect_bangresponse : card_effect {
        bool can_respond(player *origin) const;
        void on_play(player *origin);
    };
    
    struct effect_missed : card_effect {
        bool can_respond(player *origin) const;
        void on_play(player *origin);
    };

    struct effect_missedcard : effect_missed {};

    struct effect_bangmissed : card_effect {
        bool can_respond(player *origin) const;
        void on_play(player *origin);
    };
    
    struct effect_barrel : card_effect {
        bool can_respond(player *origin) const;
        void on_play(player *origin, player *target, int card_id);
    };

    struct effect_heal : card_effect {
        void on_play(player *origin, player *target);
    };

    struct effect_damage : card_effect {
        bool can_play(player *target) const;
        void on_play(player *origin, player *target);
    };

    struct effect_empty : card_effect {
        void on_play(player *origin) {}
    };

    struct effect_beer : card_effect {
        void on_play(player *origin, player *target);
    };

    struct effect_deathsave : card_effect {
        bool can_respond(player *origin) const;
        void on_play(player *origin);
    };

    struct effect_destroy : card_effect {
        void on_play(player *origin, player *target, int card_id);
    };

    struct effect_virtual_destroy : card_effect {
        void on_play(player *origin, player *target, int card_id);
    };

    struct effect_draw : card_effect {
        void on_play(player *origin, player *target);
    };

    struct effect_draw_discard : card_effect {
        bool can_play(player *target) const;
        void on_play(player *origin, player *target);
    };

    struct effect_draw_rest : card_effect {
        void on_play(player *origin, player *target);
    };

    struct effect_steal : card_effect {
        void on_play(player *origin, player *target, int card_id);
    };

    struct effect_mustang : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_scope : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_jail : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
        void on_predraw_check(player *target, int card_id);
    };

    struct effect_dynamite : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
        void on_predraw_check(player *target, int card_id);
    };

    struct effect_weapon : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_volcanic : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_generalstore : card_effect {
        void on_play(player *origin);
    };

    struct effect_boots : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_horsecharm : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_pickaxe : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_calumet : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };
}

#endif