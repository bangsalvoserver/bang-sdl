#include "holders.h"

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
                throw std::runtime_error("missing on_play(origin_card, origin)");
            }
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, player *target) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_play(origin_card, origin, target); }) {
                value.on_play(origin_card, origin, target);
            } else {
                throw std::runtime_error("missing on_play(origin_card, origin, target)");
            }
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        visit_effect([=](auto &&value) {
            if constexpr (requires { value.on_play(origin_card, origin, target, target_card); }) {
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

    game_formatted_string request_holder::status_text(player *owner) const {
        return enums::visit([owner](const auto &req) -> game_formatted_string {
            if constexpr (requires { req.status_text(owner); }) {
                return req.status_text(owner);
            }
            throw std::runtime_error("missing status_text()");
        }, *this);
    }

    bool request_holder::resolvable() const {
        return enums::visit([](auto &req) {
            return requires (std::remove_cvref_t<decltype(req)> obj) { obj.on_resolve(); };
        }, *this);
    }

    void request_holder::on_resolve() {
        enums::visit([](auto &req) {
            if constexpr (requires { req.on_resolve(); }) {
                auto copy = std::move(req);
                copy.on_resolve();
            } else {
                throw std::runtime_error("missing on_resolve()");
            }
        }, *this);
    }

    bool request_holder::can_pick(card_pile_type pile, player *target, card *target_card) const {
        return enums::visit([&](auto &req) {
            if constexpr (requires { req.can_pick(pile, target, target_card); }) {
                return req.can_pick(pile, target, target_card);
            }
            return false;
        }, *this);
    }

    void request_holder::on_pick(card_pile_type pile, player *target, card *target_card) {
        enums::visit([&](auto &req) {
            if constexpr (requires { req.on_pick(pile, target, target_card); }) {
                auto copy = req;
                copy.on_pick(pile, target, target_card);
            } else {
                throw std::runtime_error("missing on_pick(pile, target, target_card)");
            }
        }, *this);
    }

    bool request_holder::tick() {
        return enums::visit([&](auto &req) {
            if constexpr (std::is_base_of_v<timer_request, std::remove_cvref_t<decltype(req)>>) {
                if (req.duration && --req.duration == 0) {
                    if constexpr (requires { req.on_finished(); }) {
                        auto copy = std::move(req);
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
        enums::visit([](auto &req) {
            if constexpr (requires { req.cleanup(); }) {
                req.cleanup();
            }
        }, *this);
    }

}