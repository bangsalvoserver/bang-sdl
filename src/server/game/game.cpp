#include "game.h"

#include "card.h"
#include "common/responses.h"

#include <array>

namespace banggame {

    void game::add_show_card(const card &c, player *owner) {
        show_card_update obj;
        obj.card_id = c.id;
        obj.color = c.color;
        obj.image = c.image;
        obj.name = c.name;
        obj.suit = c.suit;
        obj.value = c.value;
        for (const auto &value : c.effects) {
            obj.targets.push_back(value->target);
        }

        if (!owner) {
            add_public_update<game_update_type::show_card>(obj);
        } else {
            hide_card_update hide;
            hide.card_id = c.id;

            for (auto &p : m_players) {
                if (&p == owner) {
                    add_private_update<game_update_type::show_card>(&p, obj);
                } else {
                    add_private_update<game_update_type::hide_card>(&p, hide);
                }
            }
        }
    }

    void game::start_game(const game_options &options) {
        for (int i=0; i<options.nplayers; ++i) {
            m_players.emplace_back(this);
        }

        for (auto &p : m_players) {
            add_private_update<game_update_type::player_own_id>(&p, p.id);
        }
        add_public_update<game_update_type::game_notify>(game_notify_type::game_started);
        
        std::random_device rd;
        rng.seed(rd());

        auto all_cards = read_cards(options.allowed_expansions);
        shuffle_cards_and_ids(all_cards.cards, rng);
        std::ranges::shuffle(all_cards.characters, rng);

        m_deck = all_cards.cards;

        auto character_it = all_cards.characters.begin();

        std::array roles{
            player_role::sheriff,
            player_role::outlaw,
            player_role::outlaw,
            player_role::renegade,
            player_role::deputy,
            player_role::outlaw,
            player_role::deputy,
            player_role::renegade
        };

        std::ranges::shuffle(roles.begin(), roles.begin() + options.nplayers, rng);
        auto role_it = roles.begin();

        for (auto &p : m_players) {
            p.set_character_and_role(*character_it++, *role_it++);
            for (int i=0; i<p.get_hp(); ++i) {
                p.add_to_hand(draw_card());
            }
        }

        m_playing = &*std::ranges::find(m_players, player_role::sheriff, &player::role);
        m_playing->start_of_turn();
    }

    card game::draw_card() {
        if (m_deck.empty()) {
            card top_discards = std::move(m_discards.back());
            m_deck = m_discards;
            shuffle_cards_and_ids(m_deck, rng);
            add_public_update<game_update_type::game_notify>(game_notify_type::deck_shuffled);
        }
        card c = std::move(m_deck.back());
        m_deck.pop_back();
        return c;
    }

    card game::draw_from_temp(int index) {
        card c = std::move(m_temps.at(index));
        m_temps.erase(m_temps.begin() + index);
        return c;
    }

    void game::draw_check_then(player *p, draw_check_function &&fun) {
        if (p->get_num_checks() == 1) {
            fun(&add_to_discards(draw_card()));
        } else {
            m_pending_checks.push_back(std::move(fun));
            for (int i=0; i<p->get_num_checks(); ++i) {
                add_to_temps(draw_card());
            }
            add_response<response_type::check>(nullptr, p);
        }
    }

    void game::resolve_check(int check_index) {
        for (size_t i=0; i<m_temps.size(); ++i) {
            if (i != check_index) {
                add_to_discards(std::move(m_temps[i]));
            }
        }
        card &c = m_temps.at(check_index);
        m_pending_checks.front()(&c);
        m_pending_checks.pop_front();
        add_to_discards(std::move(c));
        m_temps.clear();
    }

    void game::handle_action(enums::enum_constant<game_action_type::pick_card>, player *p, const pick_card_args &args) {
        if (!m_responses.empty() && p == top_response()->target) {
            if (auto *r = top_response().as<picking_response>()) {
                r->on_pick(args.source, args.source_value);
            }
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::play_card>, player *p, const play_card_args &args) {
        if (m_responses.empty() && m_playing == p) {
            p->play_card(args);
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::respond_card>, player *p, const play_card_args &args) {
        if (!m_responses.empty() && p == top_response()->target) {
            p->respond_card(args);
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::pass_turn>, player *p) {
        if (m_responses.empty() && m_playing == p) {
            if (p->num_hand_cards() <= p->get_hp()) {
                next_turn();
            } else {
                for (int i=p->get_hp(); i<p->num_hand_cards(); ++i) {
                    add_response<response_type::discard>(nullptr, p);
                }
            }
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::resolve>, player *p) {
        if (!m_responses.empty() && p == top_response()->target) {
            if (auto *r = top_response().as<card_response>()) {
                r->on_resolve();
            }
        }
    }
}