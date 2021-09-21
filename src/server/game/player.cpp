#include "player.h"

#include "game.h"

#include "common/effects.h"
#include "common/responses.h"

namespace banggame {
    constexpr auto is_weapon = [](const card &c) {
        return !c.effects.empty() && c.effects.front().is<effect_weapon>();
    };

    void player::discard_hand_card(int index) {
        auto it = m_hand.begin() + index;
        get_game()->add_to_discards(std::move(*it));
        m_hand.erase(it);
    }

    void player::discard_weapon() {
        auto it = std::ranges::find_if(m_table, is_weapon);
        if (it != m_table.end()) {
            m_table.erase(it);
            unequip_card(*it);
        }
    }

    void player::equip_card(card &&target) {
        get_game()->add_show_card(target);
        get_game()->add_public_update<game_update_type::move_card>(target.id, card_pile_type::player_table, id);
        card &moved = m_table.emplace_back(std::move(target));
        for (auto &e : moved.effects) {
            e->on_equip(this);
        }
    }
    
    void player::unequip_card(card &c) {
        for (auto &e : c.effects) {
            e->on_unequip(this);
        }
    }

    card player::get_card_removed(card *target) {
        card c;
        auto it = std::ranges::find(m_table, target->id, &card::id);
        if (it == m_table.end()) {
            it = std::ranges::find(m_hand, target->id, &card::id);
            c = std::move(*it);
            m_hand.erase(it);
        } else {
            if (auto inactive_it = std::ranges::find(m_inactive_cards, it->id); inactive_it != m_inactive_cards.end()) {
                m_inactive_cards.erase(inactive_it);
            }
            unequip_card(*it);
            c = std::move(*it);
            m_table.erase(it);
        }
        return c;
    }

    card &player::discard_card(card *target) {
        return get_game()->add_to_discards(get_card_removed(target));
    }

    void player::steal_card(player *target, card *target_card) {
        const card &c = m_hand.emplace_back(target->get_card_removed(target_card));
        get_game()->add_show_card(c, this);
        get_game()->add_public_update<game_update_type::move_card>(c.id, card_pile_type::player_hand, id);
    }

    void player::damage(int value) {
        m_hp -= value;
        get_game()->add_public_update<game_update_type::player_hp>(id, m_hp);
        if (m_hp <= 0) {
            get_game()->add_response<response_type::death>(nullptr, this);
        }
    }

    void player::heal(int value) {
        m_hp = std::min(m_hp + value, m_max_hp);
        get_game()->add_public_update<game_update_type::player_hp>(id, m_hp);
    }

    void player::add_to_hand(card &&target) {
        const auto &c = m_hand.emplace_back(std::move(target));
        get_game()->add_show_card(c, this);
        get_game()->add_public_update<game_update_type::move_card>(c.id, card_pile_type::player_hand, id);
    }

    player &player::get_next_player() {
        return get_game()->get_next_player(this);
    }

    void player::do_play_card(card &c, const std::vector<play_card_target> &targets) {
        if (c.effects.size() != targets.size()) {
            throw std::runtime_error("Numero di target non valido");
        }
        auto effect_it = targets.begin();
        for (auto &e : c.effects) {
            enums::visit(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::target_none>) {
                    e->on_play(this);
                },
                [&](enums::enum_constant<play_card_target_type::target_player>, const std::vector<target_player_id> &args) {
                    for (const auto &target : args) {
                        e->on_play(this, get_game()->get_player(target.player_id));
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_table_card>, const std::vector<target_table_card_id> &args) {
                    for (const auto &target : args) {
                        auto *target_player = get_game()->get_player(target.player_id);
                        auto *target_card = &target_player->m_table.at(target.card_index);
                        e->on_play(this, target_player, target_card);
                    }
                },
                [&](enums::enum_constant<play_card_target_type::target_hand_card>, const std::vector<target_player_id> &args) {
                    for (const auto &target : args) {
                        auto *target_player = get_game()->get_player(target.player_id);
                        auto *target_card = &target_player->m_hand.at(get_game()->rng() % target_player->m_hand.size());
                        e->on_play(this, target_player, target_card);
                    }
                }
            }, *effect_it);
            ++effect_it;
        }
    }

    void player::play_card(const play_card_args &args) {
        auto card_it = std::ranges::find(m_hand, args.card_id, &card::id);
        if (card_it == m_hand.end()) {
            card_it = std::ranges::find(m_table, args.card_id, &card::id);
            switch (card_it->color) {
            case card_color_type::green:
                if (std::ranges::find(m_inactive_cards, card_it->id) == m_inactive_cards.end()) {
                    card removed = std::move(*card_it);
                    m_hand.erase(card_it);
                    do_play_card(removed, args.targets);
                    get_game()->add_to_discards(std::move(removed));
                }
                break;
            default:
                throw std::runtime_error("Carta giocata non valida");
            }
        } else {
            switch (card_it->color) {
            case card_color_type::brown: {
                card removed = std::move(*card_it);
                do_play_card(removed, args.targets);
                get_game()->add_to_discards(std::move(removed));
                break;
            }
            case card_color_type::blue: {
                card removed = std::move(*card_it);
                m_hand.erase(card_it);
                equip_card(std::move(removed));
                break;
            }
            case card_color_type::green: {
                card removed = std::move(*card_it);
                int card_id = m_inactive_cards.emplace_back(removed.id);
                m_hand.erase(card_it);
                equip_card(std::move(removed));
                get_game()->add_public_update<game_update_type::tap_card>(card_id, false);
                break;
            }
            }
        }
    }
    
    void player::respond_card(const play_card_args &args) {
        auto *resp = get_game()->top_response().as<card_response>();
        if (!resp) return;

        auto card_it = std::ranges::find(m_hand, args.card_id, &card::id);
        if (card_it == m_hand.end()) {
            switch (card_it->color) {
            case card_color_type::green:
                if (std::ranges::find(m_inactive_cards, card_it->id) == m_inactive_cards.end()) {
                    if (resp->on_respond(&*card_it)) {
                        card removed = std::move(*card_it);
                        m_table.erase(card_it);
                        do_play_card(removed, args.targets);
                        get_game()->add_to_discards(std::move(removed));
                    }
                }
                break;
            case card_color_type::blue:
                resp->on_respond(&*card_it);
            default:
                throw std::runtime_error("Carta giocata non valida");
            }
        } else {
            card_it = std::ranges::find(m_table, args.card_id, &card::id);
            if (card_it != m_table.end() && card_it->color == card_color_type::brown) {
                if (resp->on_respond(&*card_it)) {
                    card removed = std::move(*card_it);
                    m_hand.erase(card_it);
                    do_play_card(removed, args.targets);
                    get_game()->add_to_discards(std::move(removed));
                }
            }
        }
    }

    void player::start_of_turn() {
        get_game()->add_public_update<game_update_type::switch_turn>(id);

        m_pending_predraw_checks = m_predraw_checks;

        get_game()->add_response<response_type::draw>(nullptr, this);
    }

    void player::end_of_turn() {
        for (int id : m_inactive_cards) {
            get_game()->add_public_update<game_update_type::tap_card>(id, true);
        }
        m_inactive_cards.clear();
        m_pending_predraw_checks.clear();
    }

    void player::discard_all() {
        for (card &c : m_table) {
            get_game()->add_to_discards(std::move(c));
        }
        for (card &c : m_hand) {
            get_game()->add_to_discards(std::move(c));
        }
    }

    void player::set_character_and_role(const character &c, player_role role) {
        m_character = c;
        m_role = role;

        m_max_hp = c.max_hp;

        m_character.effect->on_equip(this);
        get_game()->add_public_update<game_update_type::player_character>(id, c.id, c.name, c.image, c.effect->target);

        if (role == player_role::sheriff) {
            ++m_max_hp;
            get_game()->add_public_update<game_update_type::player_show_role>(id, m_role);
        } else {
            get_game()->add_private_update<game_update_type::player_show_role>(this, id, m_role);
        }

        m_hp = m_max_hp;
        get_game()->add_public_update<game_update_type::player_hp>(id, m_hp);
    }
}