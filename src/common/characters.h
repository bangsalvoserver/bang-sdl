#ifndef __CHARACTERS_H__
#define __CHARACTERS_H__

#include "effects.h"
#include "equips.h"

namespace banggame {

    struct effect_slab_the_killer : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_black_jack : card_effect {
        void on_play(int origin_card_id, player *target);
    };

    struct effect_bill_noface : card_effect {
        void on_play(int origin_card_id, player *target);
    };

    struct effect_tequila_joe : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_kit_carlson : card_effect {
        void on_play(int origin_card_id, player *target);
    };

    struct request_kit_carlson : request_base {
        void on_pick(const pick_card_args &args);
    };

    struct effect_claus_the_saint : card_effect {
        void on_play(int origin_card_id, player *target);
    };

    struct request_claus_the_saint : request_base {
        void on_pick(const pick_card_args &args);
    };

    struct effect_el_gringo : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_suzy_lafayette : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_vulture_sam : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_greg_digger : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_herb_hunter : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_johnny_kisch : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_molly_stark : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_bellestar : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_vera_custer : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct request_vera_custer : request_base {
        void on_pick(const pick_card_args &args);
    };

    struct effect_tuco_franziskaner : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_colorado_bill : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_henry_block : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_big_spencer : card_effect {
        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);
    };

    struct effect_gary_looter : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_john_pain : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_teren_kill : card_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin);
    };

    struct effect_youl_grinner : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct request_youl_grinner : request_base {
        void on_pick(const pick_card_args &args);
    };

    struct effect_flint_westwood : card_effect {
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

    struct effect_lee_van_kliff : card_effect {
        bool can_play(int origin_card_id, player *origin, player *target, int card_id) const;
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

    struct effect_don_bell : event_based_effect {
        void on_equip(player *origin, int card_id);
    };

    struct effect_madam_yto : event_based_effect {
        void on_equip(player *origin, int card_id);
    };

    struct effect_greygory_deck : event_based_effect {
        void on_play(int origin_card_id, player *origin);
        void on_equip(player *target, int card_id);
    };

    struct effect_lemonade_jim : event_based_effect {
        bool can_respond(player *origin) const;
        void on_play(int origin_card_id, player *origin, player *target);
        void on_equip(player *target, int card_id);
    };

    struct effect_dutch_will : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct request_dutch_will : request_base {
        void on_pick(const pick_card_args &args);
    };

    struct effect_josh_mccloud : card_effect {
        void on_play(int origin_card_id, player *origin);
    };

    struct request_shop_choose_target : request_base {
        void on_pick(const pick_card_args &args);
    };

    struct effect_julie_cutter : event_based_effect {
        void on_equip(player *target, int card_id);
    };

    struct effect_frankie_canton : card_effect {
        bool can_play(int origin_card_id, player *origin, player *target, int card_id) const;
        void on_play(int origin_card_id, player *origin, player *target, int card_id);
    };

}

#endif