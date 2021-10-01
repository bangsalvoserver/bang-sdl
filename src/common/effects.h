#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include "card_effect.h"
#include "characters.h"

namespace banggame {
    struct effect_bang : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_bangcard : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_banglimit : card_effect {
        virtual bool can_play(player *target) const override;
        virtual void on_play(player *origin) override;
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

    struct effect_changewws : card_effect {};

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

    struct effect_draw_rest : card_effect {
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

    struct effect_boots : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_horsecharm : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_pickaxe : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_calumet : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, effect_type,
        (bang,          effect_bang)
        (bangcard,      effect_bangcard)
        (banglimit,     effect_banglimit)
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
        (draw_rest,     effect_draw_rest)
        (generalstore,  effect_generalstore)
        (deathsave,     effect_deathsave)
        (damage,        effect_damage)
        (changewws,     effect_changewws)
        (pickaxe,       effect_pickaxe)
        (calumet,       effect_calumet)
        (boots,         effect_boots)
        (black_jack,    effect_black_jack)
        (calamity_janet, effect_calamity_janet)
        (el_gringo,     effect_el_gringo)
        (kit_carlson,   effect_kit_carlson)
        (horsecharm,    effect_horsecharm)
        (slab_the_killer, effect_slab_the_killer)
        (suzy_lafayette, effect_suzy_lafayette)
        (vulture_sam,   effect_vulture_sam)
        (claus_the_saint, effect_claus_the_saint)
        (johnny_kisch,  effect_johnny_kisch)
        (bellestar,     effect_bellestar)
        (bill_noface,   effect_bill_noface)
        (elena_fuente,  effect_elena_fuente)
        (greg_digger,   effect_greg_digger)
        (herb_hunter,   effect_herb_hunter)
        (molly_stark,   effect_molly_stark)
        (sean_mallory,  effect_sean_mallory)
        (tequila_joe,   effect_tequila_joe)
        (vera_custer,   effect_vera_custer)
    )
}

#endif