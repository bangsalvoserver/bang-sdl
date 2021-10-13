#ifndef __EFFECT_HOLDER_H__
#define __EFFECT_HOLDER_H__

#include <concepts>

#include "effects.h"
#include "equips.h"
#include "characters.h"

namespace banggame {

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
        (virtual_copy,  effect_virtual_copy)
        (virtual_clear, effect_virtual_clear)
        (steal,         effect_steal)
        (duel,          effect_duel)
        (beer,          effect_beer)
        (heal,          effect_heal)
        (indians,       effect_indians)
        (draw,          effect_draw)
        (draw_discard,  effect_draw_discard)
        (draw_rest,     effect_draw_rest)
        (draw_skip,     effect_draw_skip)
        (draw_done,     effect_draw_done)
        (generalstore,  effect_generalstore)
        (deathsave,     effect_deathsave)
        (bandidos,      effect_bandidos)
        (aimbang,       effect_aimbang)
        (poker,         effect_poker)
        (tornado,       effect_tornado)
        (damage,        effect_damage)
        (saved,         effect_saved)
        (changewws,     effect_empty)
        (black_jack,    effect_black_jack)
        (kit_carlson,   effect_kit_carlson)
        (claus_the_saint, effect_claus_the_saint)
        (bill_noface,   effect_bill_noface)
        (teren_kill,    effect_teren_kill)
        (flint_westwood, effect_flint_westwood)
        (lee_van_kliff, effect_lee_van_kliff)
        (greygory_deck, effect_greygory_deck)
        (lemonade_jim,  effect_lemonade_jim)
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
        (ghost,         effect_ghost)
        (snake,         effect_snake)
        (shotgun,       effect_shotgun)
        (bounty,        effect_bounty)
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
        (vera_custer,   effect_vera_custer)
        (tuco_franziskaner, effect_tuco_franziskaner)
        (colorado_bill, effect_colorado_bill)
        (henry_block,   effect_henry_block)
        (big_spencer,   effect_big_spencer)
        (gary_looter,   effect_gary_looter)
        (john_pain,     effect_john_pain)
        (youl_grinner,  effect_youl_grinner)
        (don_bell,      effect_don_bell)
        (madam_yto,     effect_madam_yto)
        (greygory_deck, effect_greygory_deck)
        (lemonade_jim,  effect_lemonade_jim)
    )

    namespace detail {
        template<typename T> concept is_effect = std::is_base_of_v<card_effect, T>;

        template<typename Variant> struct all_is_effect{};
        template<typename ... Ts> struct all_is_effect<std::variant<Ts...>>
            : std::bool_constant<(is_effect<Ts> && ...)> {};

        template<typename T> concept equippable = requires(T obj, player *p, int card_id) {
            obj.on_equip(p, card_id);
            obj.on_unequip(p, card_id);
        };

        template<typename Variant> struct all_equippable{};
        template<typename ... Ts> struct all_equippable<std::variant<Ts...>>
            : std::bool_constant<(equippable<Ts> && ...)> {};
        
    }

    template<enums::reflected_enum E>
    struct effect_base : enums::enum_variant<E> {
        using enums::enum_variant<E>::enum_variant;

        static_assert(detail::all_is_effect<enums::enum_variant_base<E>>::value);

        target_type target() const {
            return enums::visit([](auto tag, const card_effect &value) {
                return value.target;
            }, *this);
        }

        void set_target(target_type type) {
            enums::visit([=](auto tag, card_effect &value) {
                value.target = type;
            }, *this);
        }

        int maxdistance() const {
            return enums::visit([](auto tag, const card_effect &value) {
                return value.maxdistance;
            }, *this);
        };

        void set_maxdistance(int maxdistance) {
            enums::visit([=](auto tag, card_effect &value) {
                value.maxdistance = maxdistance;
            }, *this);
        }
    };

    struct effect_holder : effect_base<effect_type> {
        using effect_base<effect_type>::effect_base;

        bool can_play(player *target) const;
        bool can_respond(player *target) const;

        void on_play(player *origin);
        void on_play(player *origin, player *target);
        void on_play(player *origin, player *target, int card_id);
    };

    struct equip_holder : effect_base<equip_type> {
        using effect_base<equip_type>::effect_base;

        static_assert(detail::all_equippable<enums::enum_variant_base<equip_type>>::value);

        void on_equip(player *target, int card_id);
        void on_unequip(player *target, int card_id);

        void on_predraw_check(player *target, int card_id);
    };
}

#endif