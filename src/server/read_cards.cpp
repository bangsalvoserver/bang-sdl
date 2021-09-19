#include "read_cards.h"

#include <stdexcept>
#include <iostream>

#include <json/json.h>

#include "../utils/svstream.h"

#include "effects/effects.h"

extern const char __resource__bang_cards_json[];
extern const int __resource__bang_cards_json_length;

using namespace banggame;

std::unique_ptr<card_effect> static make_effect_from_json(const Json::Value &json_effect) {
    constexpr auto lut = []<effect_type ... Es>(enums::enum_sequence<Es ...>) {
        return std::array {
            +[]() -> std::unique_ptr<card_effect> {
                if constexpr (enums::has_type<Es>) {
                    return std::make_unique<enums::enum_type_t<Es>>();
                } else {
                    return nullptr;
                }
            } ...
        };
    }(enums::make_enum_sequence<effect_type>());

    const auto &name = json_effect["class"].asString();
    if (auto e = enums::from_string<effect_type>(name); e != enums::invalid_enum_value<effect_type>) {
        if (auto effect = lut[enums::indexof(e)]()) {
            if (json_effect.isMember("maxdistance")) {
                effect->maxdistance = json_effect["maxdistance"].asInt();
            }
            if (json_effect.isMember("target")) {
                effect->target = enums::from_string<target_type>(json_effect["target"].asString());
            }
            return effect;
        } else {
            return nullptr;
        }
    } else {
        throw std::runtime_error("Invalid effect class: " + name);
    }
}

std::vector<card> banggame::read_cards() {
    std::vector<card> ret;

    util::isviewstream ss({__resource__bang_cards_json, (size_t)__resource__bang_cards_json_length});

    Json::Value json_cards;
    ss >> json_cards;

    int id = 0;

    for (const auto &json_card : json_cards["cards"]) {
        card c;
        c.expansion = enums::from_string<card_expansion_type>(json_card["expansion"].asString());
        if (c.expansion != enums::invalid_enum_value<card_expansion_type>) {
            c.name = json_card["name"].asString();
            c.image = json_card["image"].asString();
            c.color = enums::from_string<card_color_type>(json_card["color"].asString());
            for (const auto &json_effect : json_card["effects"]) {
                if (auto effect = make_effect_from_json(json_effect)) {
                    c.effects.push_back(std::move(effect));
                }
            }
            for (const auto &json_sign : json_card["signs"]) {
                std::string_view str = json_sign.asString();
                c.suit = *std::ranges::find_if(enums::enum_values_v<card_suit_type>,
                    [&](card_suit_type e) {
                        return str.ends_with(enums::get_data(e));
                    }
                );
                c.value = *std::ranges::find_if(enums::enum_values_v<card_value_type>,
                    [&](card_value_type e) {
                        return str.starts_with(enums::get_data(e));
                    }
                );
                c.id = ++id;
                ret.push_back(c);
            }
        }
    }

    return ret;
}