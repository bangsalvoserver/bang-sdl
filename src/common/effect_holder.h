#ifndef __CARD_EFFECT_H__
#define __CARD_EFFECT_H__

#include <concepts>
#include <stdexcept>

#include "effects.h"
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
        (steal,         effect_steal)
        (duel,          effect_duel)
        (beer,          effect_beer)
        (heal,          effect_heal)
        (indians,       effect_indians)
        (draw,          effect_draw)
        (draw_discard,  effect_draw_discard)
        (draw_rest,     effect_draw_rest)
        (generalstore,  effect_generalstore)
        (deathsave,     effect_deathsave)
        (damage,        effect_damage)
        (changewws,     effect_changewws)
        (black_jack,    effect_black_jack)
        (kit_carlson,   effect_kit_carlson)
        (claus_the_saint, effect_claus_the_saint)
        (bill_noface,   effect_bill_noface)
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
    )

    template<enums::reflected_enum E>
    struct effect_base : enums::enum_variant<E> {
        using enums::enum_variant<E>::enum_variant;

        target_type target() const {
            return enums::visit([](auto tag, const auto &value) {
                return value.target;
            }, *this);
        }
        void set_target(target_type type) {
            enums::visit([=](auto tag, auto &value) {
                value.target = type;
            }, *this);
        }

        int maxdistance() const {
            return enums::visit([](auto tag, const auto &value) {
                return value.maxdistance;
            }, *this);
        };
        void set_maxdistance(int maxdistance) {
            enums::visit([=](auto tag, auto &value) {
                value.maxdistance = maxdistance;
            }, *this);
        }
    };

    struct effect_holder : effect_base<effect_type> {
        using effect_base<effect_type>::effect_base;

        bool can_play(player *target) const {
            return enums::visit([=](auto tag, const auto &value) {
                if constexpr (requires { value.can_play(target); }) {
                    return value.can_play(target);
                }
                return true;
            }, *this);
        }

        bool can_respond(player *target) const {
            return enums::visit([=](auto tag, const auto &value) {
                if constexpr (requires { value.can_respond(target); }) {
                    return value.can_respond(target);
                }
                return false;
            }, *this);
        }

        void on_play(player *origin) {
            enums::visit([=](auto tag, auto &value) {
                if constexpr (requires { value.on_play(origin); }) {
                    value.on_play(origin);
                } else {
                    throw std::runtime_error("on_play(origin)");
                }
            }, *this);
        }

        void on_play(player *origin, player *target) {
            enums::visit([=](auto tag, auto &value) {
                if constexpr (requires { value.on_play(origin, target); }) {
                    value.on_play(origin, target);
                } else {
                    throw std::runtime_error("on_play(origin, target)");
                }
            }, *this);
        }

        void on_play(player *origin, player *target, int card_id) {
            enums::visit([=](auto tag, auto &value) {
                if constexpr (requires { value.on_play(origin, target, card_id); }) {
                    value.on_play(origin, target, card_id);
                } else {
                    throw std::runtime_error("on_play(origin, target, card_id)");
                }
            }, *this);
        }
    };

    struct equip_holder : effect_base<equip_type> {
        using effect_base<equip_type>::effect_base;

        void on_equip(player *target, int card_id) {
            enums::visit([=](auto tag, auto &value) {
                value.on_equip(target, card_id);
            }, *this);
        }

        void on_unequip(player *target, int card_id) {
            enums::visit([=](auto tag, auto &value) {
                value.on_unequip(target, card_id);
            }, *this);
        }

        void on_predraw_check(player *target, int card_id) {
            enums::visit([=](auto tag, auto &value) {
                if constexpr (requires { value.on_predraw_check(target, card_id); }) {
                    value.on_predraw_check(target, card_id);
                } else {
                    throw std::runtime_error("on_predraw_check(target, card_id)");
                }
            }, *this);
        }
    };
}

#endif