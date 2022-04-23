#include "make_all_cards.h"

#include <stdexcept>
#include <iostream>

#include <json/json.h>
#include <fmt/core.h>

#include "utils/resource.h"
#include "utils/unpacker.h"

#include "holders.h"

namespace banggame {

    using namespace enums::flag_operators;

    struct invalid_effect : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    template<enums::reflected_enum E> E string_to_enum_or_throw(std::string_view str) {
        if (auto value = enums::from_string<E>(str)) {
            return *value;
        } else {
            throw invalid_effect(fmt::format("Invalid {}: {}", enums::enum_name_v<E>, str));
        }
    }

    static card_expansion_type get_expansion(const Json::Value &value) {
        if (value.isMember("expansion")) {
            return string_to_enum_or_throw<card_expansion_type>(value["expansion"].asString());
        }
        return {};
    }

    template<typename Holder>
    static effect_list_base<Holder> make_effects_from_json(const Json::Value &json_effects) {
        using enum_type = typename Holder::enum_type;

        effect_list_base<Holder> ret;
        for (const auto &json_effect : json_effects) {
            Holder effect;
            effect.type = string_to_enum_or_throw<enum_type>(json_effect["class"].asString());

            if (json_effect.isMember("value")) {
                effect.effect_value = json_effect["value"].asInt();
            }
            if (json_effect.isMember("target")) {
                effect.target = string_to_enum_or_throw<play_card_target_type>(json_effect["target"].asString());
            }
            if (json_effect.isMember("player_filter")) {
                if (effect.target == play_card_target_type::player || effect.target == play_card_target_type::card) {
                    effect.player_filter = string_to_enum_or_throw<target_player_filter>(json_effect["player_filter"].asString());
                } else {
                    throw invalid_effect(fmt::format("Target type {} cannot have a player filter", enums::to_string(effect.target)));
                }
            }
            if (json_effect.isMember("card_filter")) {
                if (effect.target == play_card_target_type::card) {
                    effect.card_filter = string_to_enum_or_throw<target_card_filter>(json_effect["card_filter"].asString());
                } else {
                    throw invalid_effect(fmt::format("Target type {} cannot have a card filter", enums::to_string(effect.target)));
                }
            }
            ret.push_back(effect);
        }

        return ret;
    }
    
    static void make_all_effects(card_deck_info &out, const Json::Value &json_card) {
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
            if (json_card.isMember("modifier")) {
                out.modifier = string_to_enum_or_throw<card_modifier_type>(json_card["modifier"].asString());
            }
            if (json_card.isMember("multitarget")) {
                out.multi_target_handler.type = string_to_enum_or_throw<mth_type>(json_card["multitarget"].asString());
            }
        } catch (const invalid_effect &e) {
            throw std::runtime_error(fmt::format("{}: {}", out.name, e.what()));
        }
        if (json_card.isMember("discard_if_two_players")) {
            out.discard_if_two_players = json_card["discard_if_two_players"].asBool();
        }
        if (json_card.isMember("hidden")) {
            out.hidden = json_card["hidden"].asBool();
        }
#ifndef NDEBUG
        if (json_card.isMember("testing")) {
            out.testing = json_card["testing"].asBool();
        }
#endif
    }

    all_cards_t::all_cards_t(const std::filesystem::path &base_path) {
        using namespace enums::flag_operators;

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
            card_deck_info c;
            c.deck = card_deck_type::main_deck;
            c.expansion = get_expansion(json_card);
            make_all_effects(c, json_card);
            c.color = string_to_enum_or_throw<card_color_type>(json_card["color"].asString());
            for (const auto &json_sign : json_card["signs"]) {
                std::string str = json_sign.asString();

                // assume no error
                auto space = str.find(' ');
                c.sign.suit = *enums::from_string<card_suit>(str.substr(space + 1));

                const auto &rank_letters = enums::enum_data_array_v<card_rank>;
                c.sign.rank = enums::index_to<card_rank>(std::ranges::find(rank_letters, str.substr(0, space)) - rank_letters.begin());
                
                deck.push_back(c);
            }
        }

        for (const auto &json_character : json_cards["characters"]) {
            if (is_disabled(json_character)) continue;
            card_deck_info c;
            c.deck = card_deck_type::character;
            c.expansion = get_expansion(json_character);
            make_all_effects(c, json_character);
            characters.push_back(c);
        }

        for (const auto &json_card : json_cards["goldrush"]) {
            if (is_disabled(json_card)) continue;
            card_deck_info c;
            c.deck = card_deck_type::goldrush;
            c.expansion = card_expansion_type::goldrush;
            make_all_effects(c, json_card);
            c.color = string_to_enum_or_throw<card_color_type>(json_card["color"].asString());
            if (c.hidden) {
                hidden.push_back(c);
            } else {
                int count = json_card.isMember("count") ? json_card["count"].asInt() : 1;
                for (int i=0; i<count; ++i) {
                    goldrush.push_back(c);
                }
            }
        }

        for (const auto &json_card : json_cards["highnoon"]) {
            if (is_disabled(json_card)) continue;
            card_deck_info c;
            c.deck = card_deck_type::highnoon;
            c.expansion = card_expansion_type::highnoon | get_expansion(json_card);
            make_all_effects(c, json_card);
            if (c.hidden) {
                hidden.push_back(c);
            } else {
                highnoon.push_back(c);
            }
        }

        for (const auto &json_card : json_cards["fistfulofcards"]) {
            if (is_disabled(json_card)) continue;
            card_deck_info c;
            c.deck = card_deck_type::fistfulofcards;
            c.expansion = card_expansion_type::fistfulofcards;
            make_all_effects(c, json_card);
            if (c.hidden) {
                hidden.push_back(c);
            } else {
                fistfulofcards.push_back(c);
            }
        }

        for (const auto &json_card : json_cards["wildwestshow"]) {
            if (is_disabled(json_card)) continue;
            card_deck_info c;
            c.deck = card_deck_type::wildwestshow;
            c.expansion = card_expansion_type::wildwestshow | get_expansion(json_card);
            make_all_effects(c, json_card);
            wildwestshow.push_back(c);
        }

        for (const auto &json_card : json_cards["hidden"]) {
            if (is_disabled(json_card)) continue;
            card_deck_info c;
            c.expansion = get_expansion(json_card);
            if (json_card.isMember("color")) c.color = string_to_enum_or_throw<card_color_type>(json_card["color"].asString());
            make_all_effects(c, json_card);
            hidden.push_back(c);
        }

        for (const auto &json_card : json_cards["specials"]) {
            if (is_disabled(json_card)) continue;
            card_deck_info c;
            c.expansion = get_expansion(json_card);
            make_all_effects(c, json_card);
            if (c.hidden) {
                hidden.push_back(c);
            } else {
                specials.push_back(c);
            }
        }
    };

}