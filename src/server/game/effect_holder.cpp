#include "common/effect_holder.h"

#include <stdexcept>

namespace banggame {

    bool effect_holder::can_play(player *target) const {
        return std::visit([=](const auto &value) {
            if constexpr (requires { value.can_play(target); }) {
                return value.can_play(target);
            }
            return true;
        }, this->as_base());
    }

    bool effect_holder::can_respond(player *target) const {
        return std::visit([=](const auto &value) {
            if constexpr (requires { value.can_respond(target); }) {
                return value.can_respond(target);
            }
            return false;
        }, this->as_base());
    }

    void effect_holder::on_play(player *origin) {
        std::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin); }) {
                value.on_play(origin);
            } else {
                throw std::runtime_error("on_play(origin)");
            }
        }, this->as_base());
    }

    void effect_holder::on_play(player *origin, player *target) {
        std::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin, target); }) {
                value.on_play(origin, target);
            } else {
                throw std::runtime_error("on_play(origin, target)");
            }
        }, this->as_base());
    }

    void effect_holder::on_play(player *origin, player *target, int card_id) {
        std::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin, target, card_id); }) {
                value.on_play(origin, target, card_id);
            } else {
                throw std::runtime_error("on_play(origin, target, card_id)");
            }
        }, this->as_base());
    }

    void equip_holder::on_equip(player *target, int card_id) {
        std::visit([=](auto &value) {
            value.on_equip(target, card_id);
        }, this->as_base());
    }

    void equip_holder::on_unequip(player *target, int card_id) {
        std::visit([=](auto &value) {
            value.on_unequip(target, card_id);
        }, this->as_base());
    }

}