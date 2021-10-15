#include "common/effect_holder.h"

#include <stdexcept>

namespace banggame {

    bool effect_holder::can_play(player *target) const {
        return enums::visit([=](const auto &value) {
            if constexpr (requires { value.can_play(target); }) {
                return value.can_play(target);
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

    void effect_holder::on_play(int origin_card_id, player *origin) {
        enums::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin_card_id, origin); }) {
                value.on_play(origin_card_id, origin);
            } else {
                throw std::runtime_error("on_play(origin)");
            }
        }, *this);
    }

    void effect_holder::on_play(int origin_card_id, player *origin, player *target) {
        enums::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin_card_id, origin, target); }) {
                value.on_play(origin_card_id, origin, target);
            } else {
                throw std::runtime_error("on_play(origin, target)");
            }
        }, *this);
    }

    void effect_holder::on_play(int origin_card_id, player *origin, player *target, int card_id) {
        enums::visit([=](auto &value) {
            if constexpr (requires { value.on_play(origin_card_id, origin, target, card_id); }) {
                value.on_play(origin_card_id, origin, target, card_id);
            } else {
                throw std::runtime_error("on_play(origin, target, card_id)");
            }
        }, *this);
    }

    void equip_holder::on_equip(player *target, int card_id) {
        enums::visit([=](auto &value) {
            value.on_equip(target, card_id);
        }, *this);
    }

    void equip_holder::on_unequip(player *target, int card_id) {
        enums::visit([=](auto &value) {
            value.on_unequip(target, card_id);
        }, *this);
    }

}