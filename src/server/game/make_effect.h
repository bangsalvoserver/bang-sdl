#ifndef __MAKE_EFFECT_H__
#define __MAKE_EFFECT_H__

#include <json/json.h>

#include "common/effect_holder.h"

namespace banggame {

    template<typename Holder>
    std::vector<Holder> make_effects_from_json(const Json::Value &json_effects) {
        using enum_type = typename Holder::enum_type;
        constexpr auto lut = []<enum_type ... Es>(enums::enum_sequence<Es ...>) {
            return std::array {
                +[] { return Holder(enums::enum_constant<Es>{}); } ...
            };
        }(enums::make_enum_sequence<enum_type>());

        using namespace enums::flag_operators;

        std::vector<Holder> ret;
        for (const auto &json_effect : json_effects) {
            auto e = enums::from_string<enum_type>(json_effect["class"].asString());
            if (e == enums::invalid_enum_v<enum_type>) {
                throw std::runtime_error("Invalid effect class: " + json_effect["class"].asString());
            }

            auto effect = lut[enums::indexof(e)]();
            if (json_effect.isMember("maxdistance")) {
                effect.set_maxdistance(json_effect["maxdistance"].asInt());
            }
            if (json_effect.isMember("target")) {
                effect.set_target(enums::flags_from_string<target_type>(json_effect["target"].asString()));
                if (effect.target() != enums::flags_none<target_type>
                    && !bool(effect.target() & (target_type::card | target_type::player | target_type::dead))) {
                    throw std::runtime_error("Invalid target: " + json_effect["target"].asString());
                }
            }
            ret.push_back(effect);
        }

        return ret;
    }

    extern template std::vector<equip_holder> make_effects_from_json<equip_holder>(const Json::Value &value);
}

#endif