#ifndef __EFFECT_HOLDER_H__
#define __EFFECT_HOLDER_H__

#include <concepts>

#include "effects.h"
#include "equips.h"
#include "characters.h"
#include "scenarios.h"

namespace banggame {

    DEFINE_ENUM_TYPES_IN_NS(banggame, effect_type,
        (pass_turn,     effect_pass_turn)
        (resolve,       effect_resolve)
        (predraw,       effect_predraw)
        (bang,          effect_bang)
        (bangcard,      effect_bangcard)
        (banglimit,     effect_banglimit)
        (missed,        effect_missed)
        (missedcard,    effect_missedcard)
        (bangresponse,  effect_bangresponse)
        (bangmissed,    effect_bangmissed)
        (bangresponse_onturn, effect_bangresponse_onturn)
        (barrel,        effect_barrel)
        (destroy,       effect_destroy)
        (choose_card, effect_choose_card)
        (startofturn,   effect_startofturn)
        (repeatable,    effect_empty)
        (drawing,       effect_drawing)
        (steal,         effect_steal)
        (duel,          effect_duel)
        (beer,          effect_beer)
        (heal,          effect_heal)
        (heal_notfull,  effect_heal_notfull)
        (indians,       effect_indians)
        (draw,          effect_draw)
        (draw_atend,    effect_draw_atend)
        (draw_discard,  effect_draw_discard)
        (draw_rest,     effect_draw_rest)
        (draw_skip,     effect_draw_skip)
        (draw_done,     effect_draw_done)
        (draw_again_if_needed, effect_draw_again_if_needed)
        (generalstore,  effect_generalstore)
        (deathsave,     effect_deathsave)
        (backfire,      effect_backfire)
        (bandidos,      effect_bandidos)
        (aim,           effect_aim)
        (poker,         effect_poker)
        (tornado,       effect_tornado)
        (damage,        effect_damage)
        (saved,         effect_saved)
        (escape,        effect_escape)
        (sell_beer,     effect_sell_beer)
        (discard_black, effect_discard_black)
        (rum,           effect_rum)
        (bottle,        effect_bottle)
        (bottlechoice,  effect_shopchoice)
        (pardner,       effect_pardner)
        (pardnerchoice, effect_shopchoice)
        (goldrush,      effect_goldrush)
        (pay_cube,      effect_pay_cube)
        (add_cube,      effect_add_cube)
        (reload,        effect_reload)
        (rust,          effect_rust)
        (bandolier,     effect_bandolier)
        (belltower,     effect_belltower)
        (doublebarrel,  effect_doublebarrel)
        (thunderer,     effect_thunderer)
        (buntlinespecial, effect_buntlinespecial)
        (bigfifty,      effect_bigfifty)
        (flintlock,     effect_flintlock)
        (duck,          effect_duck)
        (squaw_destroy, effect_squaw_destroy)
        (squaw_steal,   effect_squaw_steal)
        (tumbleweed,    effect_tumbleweed)
        (changewws,     effect_empty)
        (sniper,        effect_sniper)
        (ricochet,      effect_ricochet)
        (peyotechoice,  effect_empty)
        (handcuffschoice, effect_empty)
        (teren_kill,    effect_teren_kill)
        (flint_westwood_choose, effect_flint_westwood_choose)
        (flint_westwood, effect_flint_westwood)
        (greygory_deck, effect_greygory_deck)
        (lemonade_jim,  effect_lemonade_jim)
        (josh_mccloud,  effect_josh_mccloud)
        (frankie_canton, effect_frankie_canton)
        (red_ringo,     effect_red_ringo)
        (al_preacher,   effect_al_preacher)
        (ms_abigail,    effect_ms_abigail)
    )

    DEFINE_ENUM_TYPES_IN_NS(banggame, equip_type,
        (mustang,       effect_mustang)
        (scope,         effect_scope)
        (jail,          effect_jail)
        (dynamite,      effect_dynamite)
        (horse,         effect_horse)
        (weapon,        effect_weapon)
        (volcanic,      effect_volcanic)
        (pickaxe,       effect_pickaxe)
        (calumet,       effect_calumet)
        (boots,         effect_boots)
        (ghost,         effect_ghost)
        (snake,         effect_snake)
        (shotgun,       effect_shotgun)
        (bounty,        effect_bounty)
        (el_gringo,     effect_el_gringo)
        (horsecharm,    effect_horsecharm)
        (luckycharm,    effect_luckycharm)
        (gunbelt,       effect_gunbelt)
        (wanted,        effect_wanted)
        (bomb,          effect_bomb)
        (tumbleweed,    effect_tumbleweed)
        (lemat,         effect_lemat)
        (bronco,        effect_empty)
        (calamity_janet, effect_calamity_janet)
        (black_jack,    effect_black_jack)
        (kit_carlson,   effect_kit_carlson)
        (claus_the_saint, effect_claus_the_saint)
        (bill_noface,   effect_bill_noface)
        (slab_the_killer, effect_slab_the_killer)
        (suzy_lafayette, effect_suzy_lafayette)
        (vulture_sam,   effect_vulture_sam)
        (johnny_kisch,  effect_johnny_kisch)
        (bellestar,     effect_bellestar)
        (greg_digger,   effect_greg_digger)
        (herb_hunter,   effect_herb_hunter)
        (molly_stark,   effect_molly_stark)
        (tequila_joe,   effect_tequila_joe)
        (vera_custer,   effect_vera_custer)
        (tuco_franziskaner, effect_tuco_franziskaner)
        (colorado_bill, effect_colorado_bill)
        (henry_block,   effect_henry_block)
        (big_spencer,   effect_big_spencer)
        (gary_looter,   effect_gary_looter)
        (john_pain,     effect_john_pain)
        (youl_grinner,  effect_youl_grinner)
        (don_bell,      effect_don_bell)
        (dutch_will,    effect_dutch_will)
        (madam_yto,     effect_madam_yto)
        (greygory_deck, effect_greygory_deck)
        (lemonade_jim,  effect_lemonade_jim)
        (julie_cutter,  effect_julie_cutter)
        (bloody_mary,   effect_bloody_mary)
        (red_ringo,     effect_red_ringo)
        (al_preacher,   effect_al_preacher)
        (blessing,      effect_blessing)
        (curse,         effect_curse)
        (thedaltons,    effect_thedaltons)
        (thedoctor,     effect_thedoctor)
        (trainarrival,  effect_trainarrival)
        (thirst,        effect_thirst)
        (highnoon,      effect_highnoon)
        (shootout,      effect_shootout)
        (invert_rotation, effect_invert_rotation)
        (reverend,      effect_reverend)
        (hangover,      effect_hangover)
        (sermon,        effect_sermon)
        (ghosttown,     effect_ghosttown)
        (handcuffs,     effect_handcuffs)
        (ambush,        effect_ambush)
        (lasso,         effect_lasso)
        (judge,         effect_judge)
        (peyote,        effect_peyote)
        (russianroulette, effect_russianroulette)
        (abandonedmine, effect_abandonedmine)
        (deadman,       effect_deadman)
        (fistfulofcards, effect_fistfulofcards)
    )

    namespace detail {
        template<typename T> concept is_effect = std::is_base_of_v<card_effect, T>;

        template<typename Variant> struct all_is_effect{};
        template<typename ... Ts> struct all_is_effect<std::variant<Ts...>>
            : std::bool_constant<(is_effect<Ts> && ...)> {};
    }

    template<enums::reflected_enum E>
    struct effect_base : card_effect {
        using enum_type = E;
        enum_type type;

        effect_base(enum_type type) : type(type) {}

        bool is(enum_type value) const { return type == value; }

        template<enum_type Value> auto get() const {
            if (type != Value) throw std::runtime_error("Tipo non valido");
            enums::enum_type_t<Value> value;
            static_cast<card_effect &>(value) = static_cast<const card_effect &>(*this);
            return value;
        }
        
        static_assert(detail::all_is_effect<enums::enum_variant_base<E>>::value);
    };

    struct effect_holder : effect_base<effect_type> {
        using effect_base<effect_type>::effect_base;

        bool can_respond(card *origin_card, player *target) const;

        void verify(card *origin_card, player *origin) const;
        void on_play(card *origin_card, player *origin);
        
        void verify(card *origin_card, player *origin, player *target) const;
        void on_play(card *origin_card, player *origin, player *target);
        
        void verify(card *origin_card, player *origin, player *target, card *target_card) const;
        void on_play(card *origin_card, player *origin, player *target, card *target_card);
    };

    struct equip_holder : effect_base<equip_type> {
        using effect_base<equip_type>::effect_base;

        void on_pre_equip(player *target, card *target_card);
        void on_equip(player *target, card *target_card);
        void on_unequip(player *target, card *target_card);
    };
}

#endif