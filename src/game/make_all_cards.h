#ifndef __MAKE_ALL_CARDS_H__
#define __MAKE_ALL_CARDS_H__

#include <filesystem>

#include "card_data.h"

namespace banggame {

    struct card_deck_info : card_data {
        bool discard_if_two_players = false;
        bool hidden = false;

#ifndef NDEBUG
        bool testing = false;
#endif
    };

    struct all_cards_t {
        std::vector<card_deck_info> deck;
        std::vector<card_deck_info> characters;
        std::vector<card_deck_info> goldrush;
        std::vector<card_deck_info> hidden;
        std::vector<card_deck_info> specials;
        std::vector<card_deck_info> highnoon;
        std::vector<card_deck_info> fistfulofcards;
        std::vector<card_deck_info> wildwestshow;

        all_cards_t(const std::filesystem::path &path);
    };
}

#endif