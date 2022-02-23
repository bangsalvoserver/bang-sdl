#ifndef __CARD_H__
#define __CARD_H__

#include <filesystem>
#include <vector>
#include <string>

#include "holders.h"
#include "card_enums.h"
#include "game_update.h"

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
        mth_type multi_target_handler = mth_type::none;
        std::string name;
        std::string image;
        int8_t usages = 0;
        int8_t max_usages = 0;
        bool discard_if_two_players = false;

        int8_t buy_cost = 0;
        int8_t cost = 0;
        
        card_suit_type suit = card_suit_type::none;
        card_value_type value = card_value_type::none;
        card_color_type color = card_color_type::none;
        bool inactive = false;

        std::vector<int> cubes;

        card_pile_type pile = card_pile_type::none;
        player *owner = nullptr;

#ifndef DISABLE_TESTING
        bool testing = false;
#endif
    };

    inline std::vector<int> make_id_vector(auto &&range) {
        auto view = range | std::views::transform(&card::id);
        return {view.begin(), view.end()};
    };

    struct character : card {
        int max_hp;
    };

    struct all_cards_t {
        std::vector<card> deck;
        std::vector<character> characters;
        std::vector<card> goldrush;
        std::vector<card> hidden;
        std::vector<card> specials;
        std::vector<card> highnoon;
        std::vector<card> fistfulofcards;
        std::vector<card> wildwestshow;
    };

    all_cards_t make_all_cards(const std::filesystem::path &base_path);

}

#endif