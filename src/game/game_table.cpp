#include "game_table.h"

#include "formatter.h"

namespace banggame {

    using namespace enums::flag_operators;

    card *game_table::find_card(int card_id) {
        if (auto it = m_cards.find(card_id); it != m_cards.end()) {
            return &*it;
        }
        throw game_error("server.find_card: ID not found"_nonloc);
    }

    player *game_table::find_player(int player_id) {
        if (auto it = m_players.find(player_id); it != m_players.end()) {
            return &*it;
        }
        throw game_error("server.find_player: ID not found"_nonloc);
    }
    
    std::vector<card *> &game_table::get_pocket(pocket_type pocket, player *owner) {
        switch (pocket) {
        case pocket_type::player_hand:       return owner->m_hand;
        case pocket_type::player_table:      return owner->m_table;
        case pocket_type::player_character:  return owner->m_characters;
        case pocket_type::player_backup:     return owner->m_backup_character;
        case pocket_type::main_deck:         return m_deck;
        case pocket_type::discard_pile:      return m_discards;
        case pocket_type::selection:         return m_selection;
        case pocket_type::shop_deck:         return m_shop_deck;
        case pocket_type::shop_selection:    return m_shop_selection;
        case pocket_type::shop_discard:      return m_shop_discards;
        case pocket_type::hidden_deck:       return m_hidden_deck;
        case pocket_type::scenario_deck:     return m_scenario_deck;
        case pocket_type::scenario_card:     return m_scenario_cards;
        case pocket_type::specials:          return m_specials;
        default: throw std::runtime_error("Invalid pocket");
        }
    }

    player *game_table::get_next_player(player *p) {
        auto it = m_players.find(p->id);
        do {
            if (++it == m_players.end()) it = m_players.begin();
        } while(!it->alive());
        return &*it;
    }

    player *game_table::get_next_in_turn(player *p) {
        auto it = m_players.find(p->id);
        do {
            if (has_scenario(scenario_flags::invert_rotation)) {
                if (it == m_players.begin()) it = m_players.end();
                --it;
            } else {
                ++it;
                if (it == m_players.end()) it = m_players.begin();
            }
        } while(!it->alive() && !has_scenario(scenario_flags::ghosttown)
            && !(has_scenario(scenario_flags::deadman) && &*it == m_first_dead));
        return &*it;
    }

    int game_table::calc_distance(player *from, player *to) {
        if (from == to) return 0;
        if (from->check_player_flags(player_flags::disable_player_distances)) return to->m_distance_mod;
        if (from->check_player_flags(player_flags::see_everyone_range_1)) return 1;
        int d1=0, d2=0;
        for (player *counter = from; counter != to; counter = get_next_player(counter), ++d1);
        for (player *counter = to; counter != from; counter = get_next_player(counter), ++d2);
        return std::min(d1, d2) + to->m_distance_mod;
    }

    int game_table::num_alive() const {
        return std::ranges::count_if(m_players, &player::alive);
    }
    
    bool game_table::has_scenario(scenario_flags type) const {
        using namespace enums::flag_operators;
        return bool(m_scenario_flags & type);
    }

    void game_table::shuffle_cards_and_ids(std::vector<card *> &vec) {
        for (size_t i = vec.size() - 1; i > 0; --i) {
            size_t i2 = std::uniform_int_distribution<size_t>{0, i}(rng);
            if (i == i2) continue;

            std::swap(vec[i], vec[i2]);
            auto a = m_cards.extract(vec[i]->id);
            auto b = m_cards.extract(vec[i2]->id);
            std::swap(a->id, b->id);
            m_cards.insert(std::move(a));
            m_cards.insert(std::move(b));
        }
    }

    void game_table::send_card_update(const card &c, player *owner, show_card_flags flags) {
        if (!owner || bool(flags & show_card_flags::show_everyone)) {
            add_public_update<game_update_type::show_card>(c, flags);
        } else {
            add_public_update<game_update_type::hide_card>(c.id, flags, owner->id);
            add_private_update<game_update_type::show_card>(owner, c, flags);
        }
    }

    std::vector<card *>::iterator game_table::move_to(card *c, pocket_type pocket, bool known, player *owner, show_card_flags flags) {
        if (known) {
            send_card_update(*c, owner, flags);
        } else {
            add_public_update<game_update_type::hide_card>(c->id, flags);
        }
        auto &prev_pile = get_pocket(c->pocket, c->owner);
        auto card_it = std::ranges::find(prev_pile, c);
        if (c->pocket == pocket && c->owner == owner) {
            return std::next(card_it);
        }
        add_public_update<game_update_type::move_card>(c->id, owner ? owner->id : 0, pocket, flags);
        get_pocket(pocket, owner).emplace_back(c);
        c->pocket = pocket;
        c->owner = owner;
        return prev_pile.erase(card_it);
    }

    card *game_table::draw_card_to(pocket_type pocket, player *owner, show_card_flags flags) {
        card *drawn_card = m_deck.back();
        move_to(drawn_card, pocket, true, owner, flags);
        if (m_deck.empty()) {
            card *top_discards = m_discards.back();
            m_discards.pop_back();
            m_deck = std::move(m_discards);
            for (card *c : m_deck) {
                c->pocket = pocket_type::main_deck;
                c->owner = nullptr;
            }
            m_discards.clear();
            m_discards.emplace_back(top_discards);
            shuffle_cards_and_ids(m_deck);
            add_public_update<game_update_type::deck_shuffled>(pocket_type::main_deck);
            add_log("LOG_DECK_RESHUFFLED");
        }
        return drawn_card;
    }

    card *game_table::draw_phase_one_card_to(pocket_type pocket, player *owner, show_card_flags flags) {
        if (!has_scenario(scenario_flags::abandonedmine) || m_discards.empty()) {
            return draw_card_to(pocket, owner, flags);
        } else {
            card *drawn_card = m_discards.back();
            move_to(drawn_card, pocket, true, owner, flags);
            return drawn_card;
        }
    }

    card *game_table::draw_shop_card() {
        card *drawn_card = m_shop_deck.back();
        move_to(drawn_card, pocket_type::shop_selection);
        if (drawn_card->modifier == card_modifier_type::shopchoice) {
            for (card *c : m_hidden_deck) {
                if (!c->effects.empty() && c->effects.front().type == drawn_card->effects.front().type) {
                    send_card_update(*c, nullptr, show_card_flags::no_animation);
                }
            }
        }
        if (m_shop_deck.empty()) {
            m_shop_deck = std::move(m_shop_discards);
            for (card *c : m_shop_deck) {
                c->pocket = pocket_type::shop_deck;
                c->owner = nullptr;
            }
            m_shop_discards.clear();
            shuffle_cards_and_ids(m_shop_deck);
            add_public_update<game_update_type::deck_shuffled>(pocket_type::shop_deck);
        }
        return drawn_card;
    }

    void game_table::draw_scenario_card() {
        if (m_scenario_deck.empty()) return;

        if (m_scenario_deck.size() > 1) {
            send_card_update(**(m_scenario_deck.rbegin() + 1), nullptr, show_card_flags::no_animation);
        }
        if (!m_scenario_cards.empty()) {
            m_first_player->unequip_if_enabled(m_scenario_cards.back());
            m_scenario_flags = {};
        }
        move_to(m_scenario_deck.back(), pocket_type::scenario_card);
        m_first_player->equip_if_enabled(m_scenario_cards.back());
    }
}