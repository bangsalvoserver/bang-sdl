#ifndef __CARD_H__
#define __CARD_H__

#include <vector>
#include <string>

#include "common/effect_holder.h"
#include "common/card_enums.h"
#include "common/game_update.h"

namespace banggame {

    struct player;

    struct card {
        int id;
        card_expansion_type expansion;
        std::vector<effect_holder> effects;
        std::vector<effect_holder> responses;
        std::vector<effect_holder> optionals;
        std::vector<equip_holder> equips;
        card_modifier_type modifier = card_modifier_type::none;
        std::string name;
        std::string image;
        int usages = 0;
        int max_usages = 0;
        bool playable_offturn = false;
        bool discard_if_two_players = false;
        bool optional_repeatable = false;

        int buy_cost = 0;
        int cost = 0;
        
        card_suit_type suit = card_suit_type::none;
        card_value_type value = card_value_type::none;
        card_color_type color = card_color_type::none;
        bool inactive = false;
        bool active_when_drawing = false;

        std::vector<int> cubes;

        card_pile_type pile = card_pile_type::none;
        player *owner = nullptr;
        bool testing = false;

        void on_equip(player *target) {
            for (auto &e : equips) {
                e.on_equip(target, this);
            }
        }

        void on_unequip(player *target) {
            for (auto &e : equips) {
                e.on_unequip(target, this);
            }
        }
    };

    struct character : card {
        int max_hp;
    };

    struct all_cards_t {
        std::vector<card> deck;
        std::vector<character> characters;
        std::vector<card> goldrush;
        std::vector<card> goldrush_choices;
        std::vector<card> highnoon;
        std::vector<card> fistfulofcards;
        std::vector<card> wildwestshow;
    };

    extern const all_cards_t all_cards;

}

#endif