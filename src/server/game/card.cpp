#include "card.h"

#include <stdexcept>
#include <iostream>

#include <json/json.h>

#include "utils/svstream.h"
#include "utils/resource.h"

#include "common/effects.h"

DECLARE_RESOURCE(bang_cards_json)

using namespace banggame;

effect_holder static make_effect_from_json(const Json::Value &json_effect) {
    constexpr auto lut = []<effect_type ... Es>(enums::enum_sequence<Es ...>) {
        return std::array {
            +[]() -> effect_holder {
                if constexpr (enums::has_type<Es>) {
                    return effect_holder::make<enums::enum_type_t<Es>>();
                } else {
                    return effect_holder();
                }
            } ...
        };
    }(enums::make_enum_sequence<effect_type>());

    auto e = enums::from_string<effect_type>(json_effect["class"].asString());
    if (e == enums::invalid_enum_v<effect_type>) {
        throw std::runtime_error("Invalid effect class: " + json_effect["class"].asString());
    }

    auto effect = lut[enums::indexof(e)]();
    if (json_effect.isMember("maxdistance")) {
        effect->maxdistance = json_effect["maxdistance"].asInt();
    }
    if (json_effect.isMember("target")) {
        effect->target = enums::from_string<target_type>(json_effect["target"].asString());
    }
    return effect;
}

all_cards banggame::read_cards(card_expansion_type allowed_expansions) {
    using namespace enums::flag_operators;

    all_cards ret;

    util::isviewstream ss({RESOURCE_NAME(bang_cards_json), (size_t) RESOURCE_LENGTH(bang_cards_json)});

    Json::Value json_cards;
    ss >> json_cards;

    int id = 0;
    for (const auto &json_card : json_cards["cards"]) {
        card c;
        c.expansion = enums::from_string<card_expansion_type>(json_card["expansion"].asString());
        if (bool(c.expansion & allowed_expansions)) {
            c.name = json_card["name"].asString();
            c.image = json_card["image"].asString();
            c.color = enums::from_string<card_color_type>(json_card["color"].asString());
            for (const auto &json_effect : json_card["effects"]) {
                c.effects.push_back(make_effect_from_json(json_effect));
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
                ret.cards.push_back(c);
            }
        }
    }

    for (const auto &json_character : json_cards["characters"]) {
        character c;
        c.expansion = enums::from_string<card_expansion_type>(json_character["expansion"].asString());
        if (bool(c.expansion & allowed_expansions)) {
            c.name = json_character["name"].asString();
            c.image = json_character["image"].asString();
            if (json_character.isMember("type")) {
                c.type = enums::from_string<character_type>(json_character["type"].asString());
            }
            for (const auto &json_effect : json_cards["effects"]) {
                c.effects.push_back(make_effect_from_json(json_effect));
            }
            c.max_hp = json_character["hp"].asInt();
            c.id = ++id;
            ret.characters.push_back(c);
        }
    }

    return ret;
}