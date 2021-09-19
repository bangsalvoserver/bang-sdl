#include "player.h"

#include "effects/effects.h"
#include "game.h"

namespace banggame {
    constexpr auto is_weapon = [](const card &c) {
        if (c.effects.empty()) return false;
        return dynamic_cast<effect_weapon *>(c.effects.front().get()) != nullptr;
    };

    void player::discard_weapon() {
        auto it = std::ranges::find_if(m_table, is_weapon);
        // it->on_unequip(this)
        m_table.erase(it);
    }

    void player::equip_card(card &&target) {
        m_table.push_back(std::move(target));
    }

    card player::get_card_removed(card *target) {
        card c;
        auto it = std::ranges::find(m_table, target->id, &card::id);
        if (it == m_table.end()) {
            it = std::ranges::find(m_hand, target->id, &card::id);
            // it->on_unequip(this)
            c = std::move(*it);
            m_hand.erase(it);
        } else {
            c = std::move(*it);
            m_table.erase(it);
        }
        return c;
    }

    card &player::discard_card(card *target) {
        return get_game()->add_to_discards(get_card_removed(target));
    }

    void player::steal_card(player *target, card *target_card) {
        auto &card = m_hand.emplace_back(target->get_card_removed(target_card));
        // card.on_equip(this)
    }

    void player::damage(int value) {
        m_hp -= value;
        if (m_hp <= 0) {
            // do death state
        }
    }

    void player::heal(int value) {
        m_hp = std::min(m_hp + value, m_max_hp);
    }

    void player::play_card(card *card, player *target) {
        // TODO
    }

    void player::draw_card() {
        m_hand.emplace_back(get_game()->draw_card());
    }

    player &player::get_next_player() {
        return get_game()->get_next_player(this);
    }
}