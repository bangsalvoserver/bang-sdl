#ifndef __EFFECTS_H__
#define __EFFECTS_H__

#include "card_enums.h"
#include "game_action.h"
#include "format_str.h"

#include <ranges>

namespace banggame {

    struct player;
    struct card;

    struct card_effect {
        target_type target = enums::flags_none<target_type>;
        short args = 0;
        effect_flags flags = no_effect_flags;
    };

    struct request_base {
        request_base(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags)
            : origin_card(origin_card), origin(origin), target(target), flags(flags) {}

        card *origin_card;
        player *origin;
        player *target;
        effect_flags flags;
    };

    template<card_pile_type ... Es>
    struct allowed_piles {
        static bool valid_pile(card_pile_type pile) {
            return ((pile == Es) || ...);
        }
    };

    struct effect_pass_turn : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_resolve : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_predraw : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };
    
    struct effect_bang : card_effect {
        void on_play(card *origin_card, player *origin) {}
        
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_bangcard : card_effect {
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_aim : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_banglimit : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_indians : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_duel : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_bangresponse : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bangresponse_onturn : effect_bangresponse {
        bool can_respond(card *origin_card, player *origin) const;
    };
    
    struct effect_missed : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_missedcard : effect_missed {
        void verify(card *origin_card, player *origin) const;
    };

    struct effect_bangmissed : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };
    
    struct effect_barrel : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *target);
    };

    struct effect_heal : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_heal_notfull : effect_heal {
        void verify(card *origin_card, player *origin, player *target) const;
    };

    struct effect_damage : card_effect {
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_empty : card_effect {
        void on_play(card *origin_card, player *origin) {}
    };

    struct effect_beer : card_effect {
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_deathsave : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_destroy : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_choose_card : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_startofturn : effect_empty {
        void verify(card *origin_card, player *origin) const;
    };

    struct effect_draw : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_draw_atend : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_draw_discard : card_effect {
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_draw_rest : card_effect {
        void on_play(card *origin_card, player *target);
    };

    struct effect_draw_done : card_effect {
        void on_play(card *origin_card, player *target);
    };

    struct effect_draw_again_if_needed : card_effect {
        void on_play(card *origin_card, player *target);
    };

    struct effect_draw_skip : card_effect {
        void verify(card *origin_card, player *target) const;
        void on_play(card *origin_card, player *target);
    };

    struct effect_drawing : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin) {}
    };

    struct effect_steal : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_generalstore : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_backfire : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bandidos : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_tornado : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_poker : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_saved : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_escape : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_sell_beer : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_discard_black : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card);
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_rum : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_goldrush : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bottle : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_pardner : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_shopchoice : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_pay_cube : card_effect {
        void verify(card *origin_card, player *origin) const {
            verify(origin_card, origin, origin, origin_card);
        }

        void verify(card *origin_card, player *origin, player *target, card *target_card) const;

        void on_play(card *origin_card, player *origin) {
            on_play(origin_card, origin, origin, origin_card);
        }

        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_add_cube : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };
    
    struct effect_reload : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_rust : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_bandolier : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_belltower : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_doublebarrel : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_thunderer : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_buntlinespecial : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_bigfifty : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_flintlock : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_duck : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_squaw_destroy : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_squaw_steal : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_sniper : card_effect {
        void on_play(card *origin_card, player *origin, player *target);
    };

    struct effect_ricochet : card_effect {
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_lemat : card_effect {
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };

    struct effect_graverobber : card_effect {
        void on_play(card *origin_card, player *origin);
    };

    struct effect_mirage : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct effect_disarm : card_effect {
        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

    struct card_sharper_handler {
        card *origin_card;
        player *origin;
        player *target;
        card *chosen_card;
        card *target_card;

        void operator()(player *p, card *c);
        void on_resolve();
    };

    struct effect_card_sharper_choose : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_card_sharper_switch : card_effect {
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct effect_sacrifice : card_effect {
        bool can_respond(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
    };

}

#endif