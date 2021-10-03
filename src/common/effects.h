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

    struct effect_bangresponse : card_effect {
        virtual bool can_respond(player *origin) const override;
        virtual void on_play(player *origin) override;
    };
    
    struct effect_missed : card_effect {
        virtual bool can_respond(player *origin) const override;
        virtual void on_play(player *origin) override;
    };

    struct effect_missedcard : effect_missed {};

    struct effect_bangmissed : card_effect {
        virtual bool can_respond(player *origin) const override;
        virtual void on_play(player *origin) override;
    };
    
    struct effect_barrel : card_effect {
        virtual bool can_respond(player *origin) const override;
        virtual void on_play(player *origin, player *target, int card_id) override;
    };

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

    struct effect_deathsave : card_effect {
        virtual bool can_respond(player *origin) const override;
        virtual void on_play(player *origin) override;
    };

    struct effect_destroy : card_effect {
        virtual void on_play(player *origin, player *target, int card_id) override;
    };

    struct effect_virtual_destroy : card_effect {
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

    struct effect_mustang : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_scope : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_jail : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
        virtual void on_predraw_check(player *target, int card_id) override;
    };

    struct effect_dynamite : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
        virtual void on_predraw_check(player *target, int card_id) override;
    };

    struct effect_weapon : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_volcanic : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_generalstore : card_effect {
        virtual void on_play(player *origin) override;
    };

    struct effect_boots : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_horsecharm : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_pickaxe : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_calumet : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, effect_type,
        (bang,          effect_bang)
        (bangcard,      effect_bangcard)
        (banglimit,     effect_banglimit)
        (missed,        effect_missed)
        (missedcard,    effect_missedcard)
        (bangresponse,  effect_bangresponse)
        (bangmissed,    effect_bangmissed)
        (barrel,        effect_barrel)
        (destroy,       effect_destroy)
        (virtual_destroy, effect_virtual_destroy)
        (steal,         effect_steal)
        (duel,          effect_duel)
        (beer,          effect_beer)
        (heal,          effect_heal)
        (indians,       effect_indians)
        (draw,          effect_draw)
        (draw_discard,  effect_draw_discard)
        (draw_rest,     effect_draw_rest)
        (generalstore,  effect_generalstore)
        (deathsave,     effect_deathsave)
        (damage,        effect_damage)
        (changewws,     effect_changewws)
        (black_jack,    effect_black_jack)
        (kit_carlson,   effect_kit_carlson)
        (claus_the_saint, effect_claus_the_saint)
        (bill_noface,   effect_bill_noface)
        (vera_custer,   effect_vera_custer)
    )

    DEFINE_ENUM_TYPES_IN_NS(banggame, equip_type,
        (mustang,       effect_mustang)
        (scope,         effect_scope)
        (jail,          effect_jail)
        (dynamite,      effect_dynamite)
        (weapon,        effect_weapon)
        (volcanic,      effect_volcanic)
        (pickaxe,       effect_pickaxe)
        (calumet,       effect_calumet)
        (boots,         effect_boots)
        (el_gringo,     effect_el_gringo)
        (horsecharm,    effect_horsecharm)
        (slab_the_killer, effect_slab_the_killer)
        (suzy_lafayette, effect_suzy_lafayette)
        (vulture_sam,   effect_vulture_sam)
        (johnny_kisch,  effect_johnny_kisch)
        (bellestar,     effect_bellestar)
        (greg_digger,   effect_greg_digger)
        (herb_hunter,   effect_herb_hunter)
        (molly_stark,   effect_molly_stark)
        (sean_mallory,  effect_sean_mallory)
        (tequila_joe,   effect_tequila_joe)
    )
}

#endif