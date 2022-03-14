#include "holders.h"

#include "formatter.h"

#include <stdexcept>

#include "effects/effects.h"
#include "effects/equips.h"
#include "effects/characters.h"
#include "effects/scenarios.h"

#include "game.h"

namespace banggame {

    template<typename Holder, typename Function>
    static auto visit_effect(Function &&fun, Holder &holder) {
        using enum_type = typename Holder::enum_type;
        return enums::visit_enum([&](auto enum_const) {
            using type = enums::enum_type_t<decltype(enum_const)::value>;
            if constexpr (requires { type{holder.effect_value}; }) {
                return fun(type{holder.effect_value});
            } else {
                return fun(type{});
            }
        }, holder.type);
    }

    void effect_holder::verify(card *origin_card, player *origin) const {
        visit_effect([=](const auto &value) {
            if constexpr (requires { value.verify(origin_card, origin); }) {
                value.verify(origin_card, origin);
            }
        }, *this);
    }

    void effect_holder::verify(card *origin_card, player *origin, player *target) const {
        visit_effect([=](const auto &value) {
            if constexpr (requires { value.verify(origin_card, origin, target); }) {
                value.verify(origin_card, origin, target);
            }
        }, *this);
    }

    void effect_holder::verify(card *origin_card, player *origin, player *target, card *target_card) const {
        visit_effect([=](const auto &value) {
            if constexpr (requires { value.verify(origin_card, origin, target, target_card); }) {
                value.verify(origin_card, origin, target, target_card);
            }
        }, *this);
    }

    bool effect_holder::can_respond(card *origin_card, player *target) const {
        return visit_effect([=](const auto &value) {
            if constexpr (requires { value.can_respond(origin_card, target); }) {
                return value.can_respond(origin_card, target);
            }
            return true;
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, effect_flags flags) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_play(origin_card, origin, flags); }) {
                value.on_play(origin_card, origin, flags);
            } else if constexpr (requires { value.on_play(origin_card, origin); }) {
                value.on_play(origin_card, origin);
            } else {
                throw std::runtime_error("missing on_play(origin_card, origin)");
            }
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_play(origin_card, origin, target, flags); }) {
                value.on_play(origin_card, origin, target, flags);
            } else if constexpr (requires {value.on_play(origin_card, origin, target); }) {
                value.on_play(origin_card, origin, target);
            } else {
                throw std::runtime_error("missing on_play(origin_card, origin, target)");
            }
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_play(origin_card, origin, target, target_card, flags); }) {
                value.on_play(origin_card, origin, target, target_card, flags);
            } else if constexpr (requires { value.on_play(origin_card, origin, target, target_card); }) {
                value.on_play(origin_card, origin, target, target_card);
            } else {
                throw std::runtime_error("missing on_play(origin_card, origin, target, target_card)");
            }
        }, *this);
    }

    void equip_holder::on_pre_equip(card *target_card, player *target) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_pre_equip(target_card, target); }) {
                value.on_pre_equip(target_card, target);
            }
        }, *this);
    }

    void equip_holder::on_equip(card *target_card, player *target) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_equip(target_card, target); }) {
                value.on_equip(target_card, target);
            }
        }, *this);
    }

    void equip_holder::on_unequip(card *target_card, player *target) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_unequip(target_card, target); }) {
                value.on_unequip(target_card, target);
            }
        }, *this);
    }
    
    void handle_multitarget(card *origin_card, player *origin, mth_target_list targets) {
        enums::visit_enum([&](auto enum_const) {
            constexpr mth_type E = decltype(enum_const)::value;
            if constexpr (enums::has_type<E>) {
                using handler_type = enums::enum_type_t<E>;
                handler_type{}.on_play(origin_card, origin, std::move(targets));
            }
        }, origin_card->multi_target_handler);
    }

    void request_base::on_pick(card_pile_type pile, player *target, card *target_card) {
        throw std::runtime_error("missing on_pick(pile, target, target_card)");
    }

    void timer_request::tick() {
        if (duration && --duration == 0) {
            on_finished();
        }
    }

    void timer_request::on_finished() {
        target->m_game->pop_request();
    }

}