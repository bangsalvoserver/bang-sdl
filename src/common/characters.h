#ifndef __CHARACTERS_H__
#define __CHARACTERS_H__

#include "effects.h"
#include "equips.h"

namespace banggame {

    struct effect_calamity_janet : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_slab_the_killer : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_black_jack : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_bill_noface : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_tequila_joe : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_kit_carlson : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_kit_carlson : request_base, allowed_piles<card_pile_type::selection> {
        request_kit_carlson(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_claus_the_saint : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_claus_the_saint : request_base, allowed_piles<card_pile_type::selection> {
        request_claus_the_saint(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_el_gringo : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_suzy_lafayette : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_vulture_sam : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_greg_digger : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_herb_hunter : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_johnny_kisch : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_molly_stark : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_bellestar : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_vera_custer : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_vera_custer : request_base, allowed_piles<card_pile_type::player_character> {
        request_vera_custer(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_tuco_franziskaner : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_colorado_bill : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_henry_block : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_big_spencer : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_gary_looter : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_john_pain : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_teren_kill : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_youl_grinner : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_youl_grinner : request_base, allowed_piles<card_pile_type::player_hand> {
        request_youl_grinner(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_flint_westwood_choose : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_flint_westwood : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_don_bell : event_based_effect {
        void on_equip(player *origin, card *target_card);
    };

    struct effect_madam_yto : event_based_effect {
        void on_equip(player *origin, card *target_card);
    };

    struct effect_greygory_deck : event_based_effect {
        void on_play(card *origin_card, player *origin);
        void on_equip(player *target, card *target_card);
    };

    struct effect_lemonade_jim : event_based_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
        void on_equip(player *target, card *target_card);
    };

    struct effect_dutch_will : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct request_dutch_will : request_base, allowed_piles<card_pile_type::selection> {
        request_dutch_will(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_josh_mccloud : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct request_shop_choose_target : request_base, allowed_piles<card_pile_type::player> {
        request_shop_choose_target(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct effect_julie_cutter : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_frankie_canton : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_bloody_mary : event_based_effect {
        void on_equip(player *target, card *target_card);
    };

    struct effect_red_ringo : event_based_effect {
        void on_equip(player *target, card *target_card);

        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_al_preacher : event_based_effect {
        void on_equip(player *target, card *target_card);

        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_ms_abigail : card_effect {
        bool can_escape(player *origin, card *origin_card, effect_flags flags) const;

        bool can_respond(card *origin_card, player *target) const;
        void on_play(card *origin_card, player *origin);
    };

}

#endif