#include "card.h"

#include <stdexcept>
#include <iostream>

#include <json/json.h>

#include "utils/svstream.h"
#include "utils/resource.h"

#include "common/effects.h"

DECLARE_RESOURCE(bang_cards_json)

using namespace banggame;
using namespace enums::flag_operators;

template<typename T, enums::reflected_enum auto Value>
T make_effect_holder() {
    if constexpr (enums::has_type<Value>) {
        return T::template make<enums::enum_type_t<Value>>();
    } else {
        return T();
    }
}

template<enums::reflected_enum E, typename T>
static auto make_effects_from_json(const Json::Value &json_effects) {
    using holder = vbase_holder<T>;

    constexpr auto lut = []<E ... Es>(enums::enum_sequence<Es ...>) {
        return std::array {
            make_effect_holder<holder, Es> ...
        };
    }(enums::make_enum_sequence<E>());

    std::vector<holder> ret;
    for (const auto &json_effect : json_effects) {
        auto e = enums::from_string<E>(json_effect["class"].asString());
        if (e == enums::invalid_enum_v<E>) {
            throw std::runtime_error("Invalid effect class: " + json_effect["class"].asString());
        }

        auto effect = lut[enums::indexof(e)]();
        if (json_effect.isMember("maxdistance")) {
            effect->maxdistance = json_effect["maxdistance"].asInt();
        }
        if (json_effect.isMember("target")) {
            effect->target = json::deserialize<target_type>(json_effect["target"].asString());
            if (effect->target != enums::flags_none<target_type>
                && !bool(effect->target & (target_type::card | target_type::player))) {
                throw std::runtime_error("Invalid target: " + json_effect["target"].asString());
            }
        }
        ret.push_back(effect);
    }

    return ret;
}

const all_cards_t banggame::all_cards = []() {
    using namespace enums::flag_operators;

    all_cards_t ret;

    util::isviewstream ss({RESOURCE_NAME(bang_cards_json), (size_t) RESOURCE_LENGTH(bang_cards_json)});

    Json::Value json_cards;
    ss >> json_cards;

    for (const auto &json_card : json_cards["cards"]) {
        deck_card c;
        c.expansion = enums::from_string<card_expansion_type>(json_card["expansion"].asString());
        if (c.expansion != enums::invalid_enum_v<card_expansion_type>) {
            c.name = json_card["name"].asString();
            c.image = json_card["image"].asString();
            c.color = enums::from_string<card_color_type>(json_card["color"].asString());
            c.effects = make_effects_from_json<effect_type, card_effect>(json_card["effects"]);
            c.responses = make_effects_from_json<effect_type, card_effect>(json_card["responses"]);
            c.equips = make_effects_from_json<equip_type, equip_effect>(json_card["equip"]);
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
                ret.deck.push_back(c);
            }
        }
    }

    for (const auto &json_character : json_cards["characters"]) {
        character c;
        c.expansion = enums::from_string<card_expansion_type>(json_character["expansion"].asString());
        if (c.expansion != enums::invalid_enum_v<card_expansion_type>) {
            c.name = json_character["name"].asString();
            c.image = json_character["image"].asString();
            if (json_character.isMember("type")) {
                c.type = enums::from_string<character_type>(json_character["type"].asString());
            }
            if (json_character.isMember("usages")) {
                c.usages = json_character["usages"].asInt();
            }
            c.effects = make_effects_from_json<effect_type, card_effect>(json_character["effects"]);
            c.responses = make_effects_from_json<effect_type, card_effect>(json_character["responses"]);
            c.equips = make_effects_from_json<equip_type, equip_effect>(json_character["equip"]);
            c.max_hp = json_character["hp"].asInt();
            ret.characters.push_back(c);
        }
    }

    return ret;
}();