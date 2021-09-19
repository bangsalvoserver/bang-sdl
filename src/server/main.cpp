#include <iostream>
#include <random>

#include "read_cards.h"

template<std::ranges::random_access_range R, typename Gen>
void shuffle_cards(R &&r, Gen &&gen) {
    auto id_view = r | std::views::transform(&banggame::card::id);
    std::vector<int> card_ids(id_view.begin(), id_view.end());
    std::ranges::shuffle(card_ids, gen);
    auto it = r.begin();
    for (int id : card_ids) {
        it->id = id;
        ++it;
    }
    std::ranges::shuffle(r, gen);
}

int main(int argc, char **argv) {
    using namespace enums::stream_operators;

    auto cards = banggame::read_cards();

    std::random_device rd;
    std::mt19937 rng{rd()};

    shuffle_cards(cards, rng);

    for (const auto &c : cards) {
        std::cout << c.id << " " << c.name << " " << c.suit << " " << c.value << std::endl;
    }
    return 0;
}