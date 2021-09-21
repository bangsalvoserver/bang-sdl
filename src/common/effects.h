#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include "card_effect.h"

namespace banggame {
    struct effect_bang : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_indians : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_duel : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };
    
    struct effect_bangcard : effect_bang {};

    struct effect_missed : card_effect {};

    struct effect_missedcard : effect_missed {};
    
    struct effect_barrel : card_effect {};

    struct effect_heal : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_beer : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_destroy : card_effect {
        virtual void on_play(player *origin, player *target_player, card *target_card) override;
    };

    struct effect_draw : card_effect {
        virtual void on_play(player *origin, player *target) override;
    };

    struct effect_steal : card_effect {
        virtual void on_play(player *origin, player *target_player, card *target_card) override;
    };

    struct effect_mustang : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
    };

    struct effect_scope : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
    };

    struct effect_jail : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
        virtual void on_predraw_check(player *target_player, card *target_card) override;
    };

    struct effect_dynamite : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
        virtual void on_predraw_check(player *target_player, card *target_card) override;
    };

    struct effect_weapon : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
    };

    struct effect_volcanic : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
    };

    struct effect_generalstore : card_effect {
        virtual void on_play(player *origin) override;
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
        (generalstore,  effect_generalstore)
        (changewws)
        (boots)
        (black_jack)
        (calamity_janet)
        (el_gringo)
        (jesse_jones)
        (kit_carlson)
        (horsecharm)
        (pedro_ramirez)
        (sid_ketchum)
        (slab_the_killer)
        (suzy_lafayette)
        (vulture_sam)
        (claus_the_saint)
        (johnny_kisch)
        (uncle_will)
        (calumet)
        (bellestar)
        (bill_noface)
        (chuck_wengam)
        (doc_holyday)
        (elena_fuente)
        (greg_digger)
        (herb_hunter)
        (jose_delgado)
        (molly_stark)
        (pat_brennan)
        (pixie_pete)
        (sean_mallory)
        (tequila_joe)
        (vera_custer)
    )
}

#endif