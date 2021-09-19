#ifndef __CARD_EFFECT_H__
#define __CARD_EFFECT_H__

#include "../../utils/enums.h"

namespace banggame {
    
    DEFINE_ENUM_IN_NS(banggame, target_type,
        (none)
        (self)
        (notself)
        (everyone)
        (others)
        (notsheriff)
        (reachable)
        (card)
    )

    struct player;
    struct player_card;

    struct card_effect {
        virtual ~card_effect() {}

        target_type target = target_type::none;
        int maxdistance = 0;

        virtual void on_equip(player *target) { }
        virtual void on_unequip(player *target) { }

        virtual bool on_play(player *origin) { return false; }
        virtual bool on_play(player *origin, player *target) { return false; }
        virtual bool on_play(player *origin, player_card *target) { return false; }

        virtual void on_resolve(player *target) {}

        virtual bool on_predraw_check(player_card *target) { return true; }
        
        virtual bool on_respond(card_effect *effect, player_card *origin, player *target) { return false; }
    };

}

#endif