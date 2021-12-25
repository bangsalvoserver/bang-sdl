#include "common/effect_holder.h"
#include "formatter.h"

#include <stdexcept>

namespace banggame {

    template<typename Holder, typename Function>
    static auto visit_effect(Function &&fun, Holder &holder) {
        using enum_type = typename Holder::enum_type;
        return enums::visit_enum([&](auto enum_const) {
            return fun(holder.template get<decltype(enum_const)::value>());
        }, holder.type);
    }

    bool effect_holder::can_play(card *origin_card, player *origin) const {
        return visit_effect([=](const auto &value) {
            if constexpr (requires { value.can_play(origin_card, origin); }) {
                return value.can_play(origin_card, origin);
            }
            return true;
        }, *this);
    }

    bool effect_holder::can_play(card *origin_card, player *origin, player *target) const {
        return visit_effect([=](const auto &value) {
            if constexpr (requires { value.can_play(origin_card, origin, target); }) {
                return value.can_play(origin_card, origin, target);
            }
            return true;
        }, *this);
    }

    bool effect_holder::can_play(card *origin_card, player *origin, player *target, card *target_card) const {
        return visit_effect([=](const auto &value) {
            if constexpr (requires { value.can_play(origin_card, origin, target, target_card); }) {
                return value.can_play(origin_card, origin, target, target_card);
            }
            return true;
        }, *this);
    }

    bool effect_holder::can_respond(card *origin_card, player *target) const {
        return visit_effect([=](const auto &value) {
            if constexpr (requires { value.can_respond(origin_card, target); }) {
                return value.can_respond(origin_card, target);
            }
            return false;
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_play(origin_card, origin); }) {
                value.on_play(origin_card, origin);
            } else {
                throw std::runtime_error("on_play(origin)");
            }
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, player *target) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_play(origin_card, origin, target); }) {
                value.on_play(origin_card, origin, target);
            } else {
                throw std::runtime_error("on_play(origin, target)");
            }
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_play(origin_card, origin, target, target_card); }) {
                value.on_play(origin_card, origin, target, target_card);
            } else {
                throw std::runtime_error("on_play(origin, target, card)");
            }
        }, *this);
    }

    void equip_holder::on_equip(player *target, card *target_card) {
        visit_effect([=](auto &&value) {
            value.on_equip(target, target_card);
        }, *this);
    }

    void equip_holder::on_unequip(player *target, card *target_card) {
        visit_effect([=](auto &&value) {
            value.on_unequip(target, target_card);
        }, *this);
    }

}