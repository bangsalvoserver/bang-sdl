#include "common/holders.h"

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

    void equip_holder::on_pre_equip(player *target, card *target_card) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_pre_equip(target, target_card); }) {
                value.on_pre_equip(target, target_card);
            }
        }, *this);
    }

    void equip_holder::on_equip(player *target, card *target_card) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_equip(target, target_card); }) {
                value.on_equip(target, target_card);
            }
        }, *this);
    }

    void equip_holder::on_unequip(player *target, card *target_card) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_unequip(target, target_card); }) {
                value.on_unequip(target, target_card);
            }
        }, *this);
    }

    card *request_holder::origin_card() const {
        return enums::visit(&request_base::origin_card, *this);
    }

    player *request_holder::origin() const {
        return enums::visit(&request_base::origin, *this);
    }

    player *request_holder::target() const {
        return enums::visit(&request_base::target, *this);
    }

    effect_flags request_holder::flags() const {
        return enums::visit(&request_base::flags, *this);
    }

    game_formatted_string request_holder::status_text() const {
        return enums::visit_indexed([]<request_type T>(enums::enum_constant<T>, const auto &req) -> game_formatted_string {
            if constexpr (requires { req.status_text(); }) {
                return req.status_text();
            } else {
                return {};
            }
        }, *this);
    }

    bool request_holder::resolvable() const {
        return enums::visit_indexed([]<request_type T>(enums::enum_constant<T>, auto) {
            return resolvable_request<T>;
        }, *this);
    }

    bool request_holder::tick() {
        return enums::visit_indexed([&]<request_type E>(enums::enum_constant<E>, auto &obj) {
            if constexpr (timer_request<E>) {
                if (obj.duration && --obj.duration == 0) {
                    if constexpr (requires { obj.on_finished(); }) {
                        auto copy = std::move(obj);
                        copy.on_finished();
                    } else {
                        return true;
                    }
                }
            }
            return false;
        }, *this);
    }

    void request_holder::cleanup() {
        enums::visit([](auto &value) {
            if constexpr (requires { value.cleanup(); }) {
                value.cleanup();
            }
        }, *this);
    }

    void request_holder::on_pick(card_pile_type pile, player *target, card *target_card) {
        enums::visit_indexed([&]<request_type E>(enums::enum_constant<E>, auto &req) {
            if constexpr (picking_request<E>) {
                if (req.valid_pile(pile)) {
                    auto req_copy = req;
                    req_copy.on_pick(pile, target, target_card);
                }
            }
        }, *this);
    }

}