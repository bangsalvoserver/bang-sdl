#include "common/effect_holder.h"

#include <stdexcept>

namespace banggame {

    bool effect_holder::can_play(card *origin_card, player *origin) const {
        return enums::visit([=](const auto &value) {
            if constexpr (requires { value.can_play(origin_card, origin); }) {
                return value.can_play(origin_card, origin);
            }
            return true;
        }, *this);
    }

    bool effect_holder::can_play(card *origin_card, player *origin, player *target) const {
        return enums::visit([=](const auto &value) {
            if constexpr (requires { value.can_play(origin_card, origin, target); }) {
                return value.can_play(origin_card, origin, target);
            }
            return true;
        }, *this);
    }

    bool effect_holder::can_play(card *origin_card, player *origin, player *target, card *target_card) const {
        return enums::visit([=](const auto &value) {
            if constexpr (requires { value.can_play(origin_card, origin, target, target_card); }) {
                return value.can_play(origin_card, origin, target, target_card);
            }
            return true;
        }, *this);
    }

    bool effect_holder::can_respond(player *target) const {
        return enums::visit([=](const auto &value) {
            if constexpr (requires { value.can_respond(target); }) {
                return value.can_respond(target);
            }
            return false;
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin) {
        enums::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin_card, origin); }) {
                value.on_play(origin_card, origin);
            } else {
                throw std::runtime_error("on_play(origin)");
            }
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, player *target) {
        enums::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin_card, origin, target); }) {
                value.on_play(origin_card, origin, target);
            } else {
                throw std::runtime_error("on_play(origin, target)");
            }
        }, *this);
    }

    void effect_holder::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        enums::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin_card, origin, target, target_card); }) {
                value.on_play(origin_card, origin, target, target_card);
            } else {
                throw std::runtime_error("on_play(origin, target, card)");
            }
        }, *this);
    }

    void equip_holder::on_equip(player *target, card *target_card) {
        enums::visit([=](auto &value) {
            value.on_equip(target, target_card);
        }, *this);
    }

    void equip_holder::on_unequip(player *target, card *target_card) {
        enums::visit([=](auto &value) {
            value.on_unequip(target, target_card);
        }, *this);
    }

}