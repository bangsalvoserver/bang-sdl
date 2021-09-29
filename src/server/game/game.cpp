#include "game.h"

#include "card.h"
#include "common/responses.h"
#include "common/net_enums.h"

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
            obj.targets.emplace_back(value->target, value->maxdistance);
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
        std::random_device rd;
        rng.seed(rd());

        for (int i=0; i<options.nplayers; ++i) {
            auto &p = m_players.emplace_back(this);
        }

        for (const auto &p : m_players) {
            add_public_update<game_update_type::player_add>(p.id);
        }
        for (auto &p : m_players) {
            add_private_update<game_update_type::player_own_id>(&p, p.id);
        }

        auto all_cards = read_cards(options.allowed_expansions);

        std::ranges::shuffle(all_cards.characters, rng);
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
        auto role_it = roles.begin();

        std::ranges::shuffle(roles.begin(), roles.begin() + options.nplayers, rng);
        for (auto &p : m_players) {
            p.set_character_and_role(*character_it++, *role_it++);
        }

        m_deck = std::move(all_cards.cards);
        auto ids_view = m_deck | std::views::transform(&card::id);
        add_public_update<game_update_type::add_cards>(std::vector(ids_view.begin(), ids_view.end()));
        shuffle_cards_and_ids(m_deck, rng);

        for (auto &p : m_players) {
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
            m_discards.resize(m_discards.size()-1);
            m_deck = std::move(m_discards);
            m_discards.clear();
            m_discards.emplace_back(std::move(top_discards));
            shuffle_cards_and_ids(m_deck, rng);
            add_public_update<game_update_type::game_notify>(game_notify_type::deck_shuffled);
        }
        card c = std::move(m_deck.back());
        m_deck.pop_back();
        return c;
    }

    card game::draw_from_discards() {
        card c = std::move(m_discards.back());
        m_discards.pop_back();
        return c;
    }

    card game::draw_from_temp(int card_id) {
        auto it = std::ranges::find(m_temps, card_id, &card::id);
        if (it == m_temps.end()) throw game_error("ID non trovato");
        card c = std::move(*it);
        m_temps.erase(it);
        return c;
    }

    void game::draw_check_then(player *p, draw_check_function &&fun) {
        if (p->get_num_checks() == 1) {
            auto &moved = add_to_discards(draw_card());
            fun(moved.suit, moved.value);
        } else {
            m_pending_checks.push_back(std::move(fun));
            for (int i=0; i<p->get_num_checks(); ++i) {
                add_to_temps(draw_card());
            }
            add_response<response_type::check>(nullptr, p);
        }
    }

    void game::resolve_check(int card_id) {
        card c = draw_from_temp(card_id);
        for (auto &c : m_temps) {
            add_to_discards(std::move(c));
        }
        m_temps.clear();
        card &moved = add_to_discards(std::move(c));
        m_pending_checks.front()(moved.suit, moved.value);
        m_pending_checks.pop_front();
    }

    void game::player_death(player *killer, player *target) {
        target->set_dead(true);
        target->discard_all();
        add_public_update<game_update_type::player_show_role>(target->id, target->role());
        if (m_playing == target) {
            next_turn();
        } else if (m_playing == killer) {
            switch (target->role()) {
            case player_role::outlaw:
                killer->add_to_hand(target->get_game()->draw_card());
                killer->add_to_hand(target->get_game()->draw_card());
                killer->add_to_hand(target->get_game()->draw_card());
                break;
            case player_role::deputy:
                if (killer->role() == player_role::sheriff) {
                    killer->discard_all();
                }
                break;
            }
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::pick_card>, player *p, const pick_card_args &args) {
        if (!m_responses.empty() && p == top_response()->target) {
            if (auto *r = top_response().as<picking_response>()) {
                r->on_pick(args.pile, args.card_id);
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
                    queue_response<response_type::discard>(nullptr, p);
                }
            }
        }
    }

    void game::handle_action(enums::enum_constant<game_action_type::resolve>, player *p) {
        if (!m_responses.empty() && p == top_response()->target) {
            top_response()->on_resolve();
        }
    }
}