#ifndef __CARD_H__
#define __CARD_H__

#include <vector>
#include <string>

#include "common/effect_holder.h"
#include "common/card_enums.h"
#include "common/game_update.h"

namespace banggame {

    struct card {
        int id;
        card_expansion_type expansion;
        std::vector<effect_holder> effects;
        std::vector<effect_holder> responses;
        std::vector<equip_holder> equips;
        std::string name;
        std::string image;
        bool playable_offturn = false;
        bool discard_if_two_players = false;

        int buy_cost = 0;
        int cost = 0;

        void on_equip(player *target) {
            for (auto &e : equips) {
                e.on_equip(target, id);
            }
        }

        void on_unequip(player *target) {
            for (auto &e : equips) {
                e.on_unequip(target, id);
            }
        }
    };

    struct deck_card : card {
        card_suit_type suit = card_suit_type::none;
        card_value_type value = card_value_type::none;
        card_color_type color;
        bool inactive = false;
    };

    struct character : card {
        character_type type = character_type::none;
        int usages = 0;
        int max_usages = 0;
        int max_hp;
    };

    struct all_cards_t {
        std::vector<deck_card> deck;
        std::vector<character> characters;
        std::vector<deck_card> goldrush;
    };

    extern const all_cards_t all_cards;

    template<typename Gen>
    void shuffle_cards_and_ids(std::vector<deck_card> &cards, Gen &&gen) {
        auto id_view = cards | std::views::transform(&banggame::deck_card::id);
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