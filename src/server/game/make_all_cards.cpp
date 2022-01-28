#include "card.h"

#include <stdexcept>
#include <iostream>

#include <json/json.h>

#include "common/svstream.h"
#include "common/resource.h"

#include "common/effects.h"

#ifdef BANG_CARDS_JSON_LINKED

DECLARE_RESOURCE(bang_cards_json)
static const auto bang_cards_resource = GET_RESOURCE(bang_cards_json);
static util::isviewstream bang_cards_stream({bang_cards_resource.data, bang_cards_resource.length});

#else

static std::ifstream bang_cards_stream(std::string(SDL_GetBasePath()) + "../resources/bang_cards.json");

#endif

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
                effect.target = enums::flags_from_string<target_type>(json_effect["target"].asString());
                if (effect.target != enums::flags_none<target_type>
                    && !bool(effect.target & (target_type::card | target_type::player | target_type::dead | target_type::cube_slot))) {
                    throw invalid_effect("Invalid target: " + json_effect["target"].asString());
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
        if (json_card.isMember("usages")) {
            out.max_usages = json_card["usages"].asInt();
        }
        if (json_card.isMember("cost")) {
            out.cost = json_card["cost"].asInt();
        }
        if (json_card.isMember("discard_if_two_players")) {
            out.discard_if_two_players = json_card["discard_if_two_players"].asBool();
        }
        if (json_card.isMember("testing")) {
            out.testing = json_card["testing"].asBool();
        }
    }

    const all_cards_t all_cards = [] {
        using namespace enums::flag_operators;

        all_cards_t ret;

        Json::Value json_cards;
        bang_cards_stream >> json_cards;

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
                    std::string_view str = json_sign.asString();
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
    }();

}