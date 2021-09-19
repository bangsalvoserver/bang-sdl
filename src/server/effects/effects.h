#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include "card_effect.h"

namespace banggame {
    struct effect_bang : card_effect {
        virtual bool on_play(player *target) override {
            return true;
        }

        virtual void on_resolve(player *target) override;
    };
    
    struct effect_bangcard : effect_bang {
        virtual bool on_respond(card_effect *effect, player_card *origin, player *target) override;
    };

    struct effect_missed : card_effect {
        virtual bool on_respond(card_effect *effect, player_card *origin, player *target) override;
    };

    struct effect_missedcard : effect_missed {};
    
    struct effect_barrel : card_effect {
        virtual bool on_respond(card_effect *effect, player_card *origin, player *target) override;
    };

    struct effect_heal : card_effect {
        virtual bool on_play(player *origin, player *target) override;
    };

    struct effect_beer : card_effect {
        virtual bool on_play(player *origin, player *target) override;
    };

    struct effect_destroy : card_effect {
        virtual bool on_play(player *origin, player_card *target) override;
    };

    struct effect_draw : card_effect {
        virtual bool on_play(player *origin, player *target) override;
    };

    struct effect_duel : card_effect {
        virtual bool on_play(player *origin, player *target) override {
            return true;
        }
    };

    struct effect_steal : card_effect {
        virtual bool on_play(player *origin, player_card *target) override;
    };

    struct effect_mustang : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
    };

    struct effect_scope : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
    };

    struct effect_indians : card_effect {
        virtual bool on_play(player *origin, player *target) override {
            return true;
        }
    };

    struct effect_jail : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
        virtual bool on_predraw_check(player_card *target) override;
    };

    struct effect_dynamite : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
        virtual bool on_predraw_check(player_card *target) override;
    };

    struct effect_weapon : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
    };

    struct effect_volcanic : card_effect {
        virtual void on_equip(player *target) override;
        virtual void on_unequip(player *target) override;
    };

    struct effect_generalstore : card_effect {};

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
    )
}

#endif