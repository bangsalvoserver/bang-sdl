#ifndef __CHARACTERS_H__
#define __CHARACTERS_H__

#include "card_effect.h"

namespace banggame {
    struct effect_slab_the_killer : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_black_jack : card_effect {
        virtual void on_play(player *target) override;
    };

    struct effect_bill_noface : card_effect {
        virtual void on_play(player *target) override;
    };

    struct effect_tequila_joe : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_sean_mallory : card_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_kit_carlson : card_effect {
        virtual void on_play(player *target) override;
    };

    struct response_kit_carlson : picking_response {
        virtual void on_pick(card_pile_type pile, int card_id) override;
    };
}

#endif