#ifndef __CHARACTERS_H__
#define __CHARACTERS_H__

#include "card_effect.h"

namespace banggame {
    struct effect_slab_the_killer : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_black_jack : card_effect {
        virtual void on_play(player *target) override;
    };

    struct effect_bill_noface : card_effect {
        virtual void on_play(player *target) override;
    };

    struct effect_tequila_joe : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_sean_mallory : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_kit_carlson : card_effect {
        virtual void on_play(player *target) override;
    };

    struct request_kit_carlson : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };

    struct effect_claus_the_saint : card_effect {
        virtual void on_play(player *target) override;
    };

    struct request_claus_the_saint : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };

    struct effect_el_gringo : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_suzy_lafayette : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_vulture_sam : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_greg_digger : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_herb_hunter : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_johnny_kisch : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_molly_stark : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_bellestar : equip_effect {
        virtual void on_equip(player *target, int card_id) override;
        virtual void on_unequip(player *target, int card_id) override;
    };

    struct effect_vera_custer : card_effect {
        virtual void on_play(player *origin) override;
    };

    struct request_vera_custer : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };
}

#endif