#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include "card_effect.h"
#include "characters.h"

namespace banggame {
    struct effect_bang : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_bangcard : card_effect {
        virtual bool can_play(player *target) const override;
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_indians : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_duel : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };
    
    struct effect_missed : card_effect {};

    struct effect_missedcard : effect_missed {};
    
    struct effect_barrel : card_effect {};

    struct effect_deathsave : card_effect {};

    struct effect_heal : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_damage : card_effect {
        virtual bool can_play(player *target) const override;
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_beer : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_destroy : card_effect {
        virtual void on_play(player *origin, player *target, int card_id) override;
    };

    struct effect_draw : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_draw_discard : card_effect {
        virtual bool can_play(player *target) const override;
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_steal : card_effect {
        virtual void on_play(player *origin, player *target, int card_id) override;
    };

    struct effect_mustang : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_scope : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_jail : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
        virtual void on_predraw_check(player *target, int card_id) override;
    };

    struct effect_dynamite : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
        virtual void on_predraw_check(player *target, int card_id) override;
    };

    struct effect_weapon : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_volcanic : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_generalstore : card_effect {
        virtual void on_play(player *origin) override;
    };

    struct effect_horsecharm : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, effect_type,
        (bang,          effect_bang)
        (bangcard,      effect_bangcard)
        (missed,        effect_missed)
        (missedcard,    effect_missedcard)
        (destroy,       effect_destroy)
        (steal,         effect_steal)
        (duel,          effect_duel)
        (beer,          effect_beer)
        (heal,          effect_heal)
        (mustang,       effect_mustang)
        (indians,       effect_indians)
        (scope,         effect_scope)
        (barrel,        effect_barrel)
        (jail,          effect_jail)
        (dynamite,      effect_dynamite)
        (weapon,        effect_weapon)
        (volcanic,      effect_volcanic)
        (draw,          effect_draw)
        (draw_discard,  effect_draw_discard)
        (generalstore,  effect_generalstore)
        (deathsave,     effect_deathsave)
        (damage,        effect_damage)
        (changewws)
        (boots)
        (black_jack)
        (calamity_janet)
        (el_gringo)
        (kit_carlson)
        (horsecharm,    effect_horsecharm)
        (slab_the_killer, effect_slab_the_killer)
        (suzy_lafayette)
        (vulture_sam)
        (claus_the_saint)
        (johnny_kisch)
        (calumet)
        (bellestar)
        (bill_noface)
        (elena_fuente)
        (greg_digger)
        (herb_hunter)
        (molly_stark)
        (sean_mallory)
        (tequila_joe)
        (vera_custer)
    )
}

#endif