#include "card.h"

#include <stdexcept>
#include <iostream>

#include <json/json.h>

#include "utils/svstream.h"
#include "utils/resource.h"

#include "common/effects.h"
#include "make_effect.h"

DECLARE_RESOURCE(bang_cards_json)

namespace banggame {
    
    static void make_all_effects(card &out, const Json::Value &json_card) {
        out.name = json_card["name"].asString();
        out.image = json_card["image"].asString();
        try {
            if (json_card.isMember("effects")) {
                out.effects = make_effects_from_json<effect_holder>(json_card["effects"]);
            }
            if (json_card.isMember("responses")) {
                out.responses = make_effects_from_json<effect_holder>(json_card["responses"]);
            }
        } catch (const invalid_effect &e) {
            throw std::runtime_error(out.name + ": " + e.what());
        }
        if (json_card.isMember("equip")) {
            out.equips = make_effects_from_json<equip_holder>(json_card["equip"]);
        }
        if (json_card.isMember("usages")) {
            out.max_usages = json_card["usages"].asInt();
        }
        if (json_card.isMember("offturn")) {
            out.playable_offturn = json_card["offturn"].asBool();
        }
        if (json_card.isMember("cost")) {
            out.cost = json_card["cost"].asInt();
        }
        if (json_card.isMember("discard_if_two_players")) {
            out.discard_if_two_players = json_card["discard_if_two_players"].asBool();
        }
        if (json_card.isMember("modifier")) {
            out.modifier = enums::from_string<card_modifier_type>(json_card["modifier"].asString());
        }
    }

    const all_cards_t all_cards = [] {
        using namespace enums::flag_operators;

        all_cards_t ret;

        util::isviewstream ss({RESOURCE_NAME(bang_cards_json), (size_t) RESOURCE_LENGTH(bang_cards_json)});

        Json::Value json_cards;
        ss >> json_cards;

        for (const auto &json_card : json_cards["cards"]) {
            deck_card c;
            c.expansion = enums::from_string<card_expansion_type>(json_card["expansion"].asString());
            if (c.expansion != enums::invalid_enum_v<card_expansion_type>) {
                if (json_card.isMember("disabled") && json_card["disabled"].asBool()) continue;
                make_all_effects(c, json_card);
                c.color = enums::from_string<card_color_type>(json_card["color"].asString());
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
                if (json_character.isMember("disabled") && json_character["disabled"].asBool()) continue;
                make_all_effects(c, json_character);
                if (json_character.isMember("type")) {
                    c.type = enums::from_string<character_type>(json_character["type"].asString());
                }
                c.max_hp = json_character["hp"].asInt();
                ret.characters.push_back(c);
            }
        }

        for (const auto &json_card : json_cards["goldrush"]) {
            deck_card c;
            c.expansion = card_expansion_type::goldrush;
            if (json_card.isMember("disabled") && json_card["disabled"].asBool()) continue;
            make_all_effects(c, json_card);
            c.color = enums::from_string<card_color_type>(json_card["color"].asString());
            c.buy_cost = json_card["buy_cost"].asInt();
            int count = json_card["count"].asInt();
            if (count == 0) {
                ret.goldrush_choices.push_back(c);
            } else {
                for (int i=0; i<count; ++i) {
                    ret.goldrush.push_back(c);
                }
            }
        }

        return ret;
    }();

}