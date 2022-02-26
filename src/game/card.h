#ifndef __CARD_H__
#define __CARD_H__

#include <filesystem>
#include <vector>
#include <string>

#include "holders.h"
#include "card_enums.h"

namespace banggame {

    struct player;

    struct card_data {REFLECTABLE(
        (int) id,

        (std::string) name,
        (std::string) image,

        (std::vector<effect_holder>) effects,
        (std::vector<effect_holder>) responses,
        (std::vector<effect_holder>) optionals,
        (std::vector<equip_holder>) equips,

        (card_expansion_type) expansion,

        (card_modifier_type) modifier,
        (mth_type) multi_target_handler,
        
        (card_suit_type) suit,
        (card_value_type) value,
        (card_color_type) color
    )

        bool discard_if_two_players = false;

#ifndef DISABLE_TESTING
        bool testing = false;
#endif
    };

    struct card : card_data {
        int8_t usages = 0;
        bool inactive = false;
        std::vector<int> cubes;

        card_pile_type pile = card_pile_type::none;
        player *owner = nullptr;
    };

    inline int get_buy_cost(const card_data *c) {
        return !c->equips.empty() && c->equips.back().is(equip_type::buy_cost)
            ? c->equips.back().args : 0;
    }

    inline std::vector<int> make_id_vector(auto &&range) {
        auto view = range | std::views::transform(&card::id);
        return {view.begin(), view.end()};
    };

    struct all_cards_t {
        std::vector<card_data> deck;
        std::vector<card_data> characters;
        std::vector<card_data> goldrush;
        std::vector<card_data> hidden;
        std::vector<card_data> specials;
        std::vector<card_data> highnoon;
        std::vector<card_data> fistfulofcards;
        std::vector<card_data> wildwestshow;
    };

    all_cards_t make_all_cards(const std::filesystem::path &base_path);

}

#endif