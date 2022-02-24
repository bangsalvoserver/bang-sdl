#include "card.h"

#include <stdexcept>
#include <iostream>

#include <json/json.h>

#include "utils/resource.h"
#include "utils/unpacker.h"

#include "holders.h"

namespace banggame {

    struct invalid_effect : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    template<typename Holder>
    static std::vector<Holder> make_effects_from_json(const Json::Value &json_effects) {
        using enum_type = typename Holder::enum_type;
        using namespace enums::flag_operators;

        std::vector<Holder> ret;
        for (const auto &json_effect : json_effects) {
            auto e = enums::from_string<enum_type>(json_effect["class"].asString());
            if (e == enums::invalid_enum_v<enum_type>) {
                throw invalid_effect("Invalid effect class: " + json_effect["class"].asString());
            }

            Holder effect{e};
            if (json_effect.isMember("args")) {
                effect.args = json_effect["args"].asInt();
            }
            if (json_effect.isMember("target")) {
                effect.target = enums::from_string<play_card_target_type>(json_effect["target"].asString());
                if (effect.target == enums::invalid_enum_v<play_card_target_type>) {
                    throw invalid_effect("Invalid target type: " + json_effect["target"].asString());
                }
            }
            if (json_effect.isMember("player_filter")) {
                if (effect.target == play_card_target_type::player || effect.target == play_card_target_type::card) {
                    effect.player_filter = enums::flags_from_string<target_player_filter>(json_effect["player_filter"].asString());
                    if (effect.player_filter == enums::invalid_enum_v<target_player_filter>) {
                        throw invalid_effect("Invalid player filter: " + json_effect["player_filter"].asString());
                    }
                } else {
                    throw invalid_effect(std::string("Target type ") + std::string(enums::to_string(effect.target)) + " cannot have a player filter");
                }
            }
            if (json_effect.isMember("card_filter")) {
                if (effect.target == play_card_target_type::card) {
                    effect.card_filter = enums::flags_from_string<target_card_filter>(json_effect["card_filter"].asString());
                    if (effect.card_filter == enums::invalid_enum_v<target_card_filter>) {
                        throw invalid_effect("Invalid card filter: " + json_effect["card_filter"].asString());
                    }
                } else {
                    throw invalid_effect(std::string("Target type ") + std::string(enums::to_string(effect.target)) + " cannot have a card filter");
                }
            }
            if (json_effect.isMember("escapable") && json_effect["escapable"].asBool()) {
                effect.flags |= effect_flags::escapable;
            }
            ret.push_back(effect);
        }

        return ret;
    }
    
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
            if (json_card.isMember("optional")) {
                out.optionals = make_effects_from_json<effect_holder>(json_card["optional"]);
            }
            if (json_card.isMember("equip")) {
                out.equips = make_effects_from_json<equip_holder>(json_card["equip"]);
            }
        } catch (const invalid_effect &e) {
            throw std::runtime_error(out.name + ": " + e.what());
        }
        if (json_card.isMember("modifier")) {
            out.modifier = enums::from_string<card_modifier_type>(json_card["modifier"].asString());
        }
        if (json_card.isMember("multitarget")) {
            out.multi_target_handler = enums::from_string<mth_type>(json_card["multitarget"].asString());
        }
        if (json_card.isMember("usages")) {
            out.max_usages = json_card["usages"].asInt();
        }
        if (json_card.isMember("cost")) {
            out.cost = json_card["cost"].asInt();
        }
        if (json_card.isMember("discard_if_two_players")) {
            out.discard_if_two_players = json_card["discard_if_two_players"].asBool();
        }
#ifndef DISABLE_TESTING
        if (json_card.isMember("testing")) {
            out.testing = json_card["testing"].asBool();
        }
#endif
    }

    all_cards_t make_all_cards(const std::filesystem::path &base_path) {
        using namespace enums::flag_operators;

        all_cards_t ret;
        Json::Value json_cards;
        {
            auto cards_pak_stream = ifstream_or_throw(base_path / "cards.pak");
            unpacker cards_pak(cards_pak_stream);
            cards_pak.seek("bang_cards");
            cards_pak_stream >> json_cards;
        }

        const auto is_disabled = [](const Json::Value &value) {
            return value.isMember("disabled") && value["disabled"].asBool();
        };

        for (const auto &json_card : json_cards["cards"]) {
            if (is_disabled(json_card)) continue;
            card c;
            c.expansion = enums::flags_from_string<card_expansion_type>(json_card["expansion"].asString());
            if (c.expansion != enums::invalid_enum_v<card_expansion_type>) {
                make_all_effects(c, json_card);
                c.color = enums::from_string<card_color_type>(json_card["color"].asString());
                for (const auto &json_sign : json_card["signs"]) {
                    std::string str = json_sign.asString();
                    c.suit = *std::ranges::find_if(enums::enum_values_v<card_suit_type>,
                        [&](card_suit_type e) {
                            return str.ends_with(enums::get_data(e).letter);
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
            if (is_disabled(json_character)) continue;
            character c;
            c.expansion = enums::from_string<card_expansion_type>(json_character["expansion"].asString());
            if (c.expansion != enums::invalid_enum_v<card_expansion_type>) {
                make_all_effects(c, json_character);
                c.max_hp = json_character["hp"].asInt();
                ret.characters.push_back(c);
            }
        }

        for (const auto &json_card : json_cards["goldrush"]) {
            if (is_disabled(json_card)) continue;
            card c;
            c.expansion = card_expansion_type::goldrush;
            make_all_effects(c, json_card);
            c.color = enums::from_string<card_color_type>(json_card["color"].asString());
            c.buy_cost = json_card["buy_cost"].asInt();
            int count = json_card["count"].asInt();
            for (int i=0; i<count; ++i) {
                ret.goldrush.push_back(c);
            }
        }

        for (const auto &json_card : json_cards["highnoon"]) {
            if (is_disabled(json_card)) continue;
            card c;
            c.expansion = card_expansion_type::highnoon;
            if (json_card.isMember("expansion")) {
                c.expansion |= enums::flags_from_string<card_expansion_type>(json_card["expansion"].asString());
            }
            make_all_effects(c, json_card);
            ret.highnoon.push_back(c);
        }

        for (const auto &json_card : json_cards["fistfulofcards"]) {
            if (is_disabled(json_card)) continue;
            card c;
            c.expansion = card_expansion_type::fistfulofcards;
            make_all_effects(c, json_card);
            ret.fistfulofcards.push_back(c);
        }

        for (const auto &json_card : json_cards["wildwestshow"]) {
            if (is_disabled(json_card)) continue;
            card c;
            c.expansion = card_expansion_type::wildwestshow;
            if (json_card.isMember("expansion")) {
                c.expansion |= enums::flags_from_string<card_expansion_type>(json_card["expansion"].asString());
            }
            make_all_effects(c, json_card);
            ret.wildwestshow.push_back(c);
        }

        for (const auto &json_card : json_cards["hidden"]) {
            if (is_disabled(json_card)) continue;
            card c;
            c.expansion = enums::flags_from_string<card_expansion_type>(json_card["expansion"].asString());
            if (json_card.isMember("color")) c.color = enums::from_string<card_color_type>(json_card["color"].asString());
            if (json_card.isMember("buy_cost")) c.buy_cost = json_card["buy_cost"].asInt();
            make_all_effects(c, json_card);
            ret.hidden.push_back(c);
        }

        for (const auto &json_card : json_cards["specials"]) {
            if (is_disabled(json_card)) continue;
            card c;
            c.expansion = enums::flags_from_string<card_expansion_type>(json_card["expansion"].asString());
            make_all_effects(c, json_card);
            ret.specials.push_back(c);
        }

        return ret;
    };

}