#ifndef __CARD_H__
#define __CARD_H__

#include <vector>
#include <string>

#include "common/card_effect.h"
#include "common/card_enums.h"
#include "common/update_enums.h"

namespace banggame {

    struct card {
        int id;
        card_expansion_type expansion;
        card_suit_type suit;
        card_value_type value;
        card_color_type color;
        std::vector<effect_holder> effects;
        std::string name;
        std::string image;
        bool inactive = false;
    };

    struct character {
        int id;
        card_expansion_type expansion;
        effect_holder effect;
        std::string name;
        std::string image;
        int max_hp;
    };

    struct all_cards {
        std::vector<card> cards;
        std::vector<character> characters;
    };
    
    all_cards read_cards(card_expansion_type allowed_expansions = enums::flags_all<card_expansion_type>);

    template<typename Gen>
    void shuffle_cards_and_ids(std::vector<card> &cards, Gen &&gen) {
        auto id_view = cards | std::views::transform(&banggame::card::id);
        std::vector<int> card_ids(id_view.begin(), id_view.end());
        std::ranges::shuffle(card_ids, gen);
        auto it = cards.begin();
        for (int id : card_ids) {
            it->id = id;
            ++it;
        }
        std::ranges::shuffle(cards, gen);
    }

}

#endif