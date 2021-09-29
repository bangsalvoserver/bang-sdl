#ifndef __GAME_H__
#define __GAME_H__

#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <random>

#include "card.h"
#include "player.h"

#include "common/update_enums.h"
#include "common/responses.h"

namespace banggame {

    using draw_check_function = std::function<void(card_suit_type, card_value_type)>;

    struct game_options {
        int nplayers = 0;
        card_expansion_type allowed_expansions = enums::flags_all<card_expansion_type>;
    };

    struct game {
        std::list<std::pair<player *, game_update>> m_updates;
        std::list<std::pair<response_type, response_holder>> m_responses;
        std::list<draw_check_function> m_pending_checks;
        
        std::vector<card> m_deck;
        std::vector<card> m_discards;
        std::vector<card> m_temps;
        std::vector<player> m_players;

        player *m_playing = nullptr;

        std::mt19937 rng;

        template<game_update_type E, typename ... Ts>
        void add_public_update(const Ts & ... args) {
            for (auto &p : m_players) {
                add_private_update<E>(&p, args ...);
            }
        }

        template<game_update_type E, typename ... Ts>
        void add_private_update(player *p, Ts && ... args) {
            m_updates.emplace_back(std::piecewise_construct,
                std::make_tuple(p),
                std::make_tuple(enums::enum_constant<E>{}, std::forward<Ts>(args) ...));
        }

        void add_show_card(const card &c, player *owner = nullptr);

        void start_game(const game_options &options);

        response_holder &top_response() {
            return m_responses.front().second;
        }

        void add_response_update() {
            const auto &resp = m_responses.front();
            add_public_update<game_update_type::response_handle>(resp.first,
                resp.second->origin ? resp.second->origin->id : 0,
                resp.second->target ? resp.second->target->id : 0);
        }

        template<response_type E>
        auto *add_response(player *origin, player *target) {
            auto &ret = m_responses.emplace_front(E, response_holder::make<enums::enum_type_t<E>>()).second;
            ret->origin = origin;
            ret->target = target;

            add_response_update();

            return ret.template as<enums::enum_type_t<E>>();
        }

        template<response_type E>
        auto *queue_response(player *origin, player *target) {
            auto &ret = m_responses.emplace_back(E, response_holder::make<enums::enum_type_t<E>>()).second;
            ret->origin = origin;
            ret->target = target;

            if (m_responses.size() == 1) {
                add_response_update();
            }

            return ret.template as<enums::enum_type_t<E>>();
        }

        void pop_response() {
            m_responses.pop_front();
            if (m_responses.empty()) {
                add_public_update<game_update_type::response_done>();
            } else {
                add_response_update();
            }
        }

        card &add_to_discards(card &&c) {
            add_show_card(c);
            add_public_update<game_update_type::move_card>(c.id, 0, card_pile_type::discard_pile);
            return m_discards.emplace_back(std::move(c));
        }

        card &add_to_temps(card &&c) {
            add_show_card(c);
            add_public_update<game_update_type::move_card>(c.id, 0, card_pile_type::temp_table);
            return m_temps.emplace_back(std::move(c));
        }

        card draw_card();
        card draw_from_discards();
        card draw_from_temp(int card_id);

        void draw_check_then(player *p, draw_check_function &&fun);
        void resolve_check(int card_id);

        int num_alive() const {
            return std::ranges::count_if(m_players, &player::alive);
        }

        player *get_next_player(player *p) {
            auto it = m_players.begin() + (p - m_players.data());
            do {
                if (++it == m_players.end()) it = m_players.begin();
            } while(!it->alive());
            return &*it;
        }

        player *get_player(int id) {
            auto it = std::ranges::find(m_players, id, &player::id);
            if (it != m_players.end()) {
                return &*it;
            } else {
                return nullptr;
            }
        }

        int calc_distance(player *from, player *to) {
            if (from == to) return 0;
            int d1=0, d2=0;
            for (player *counter = from; counter != to; counter = get_next_player(counter), ++d1);
            for (player *counter = to; counter != from; counter = get_next_player(counter), ++d2);
            return std::min(d1, d2) + to->get_distance() - from->get_range();
        }

        void next_turn() {
            if (m_playing->alive()) {
                m_playing->end_of_turn();
            }
            if (num_alive() > 0) {
                m_playing = get_next_player(m_playing);
                m_playing->start_of_turn();
            }
        }

        void player_death(player *killer, player *target);

        void handle_action(enums::enum_constant<game_action_type::pick_card>, player *p, const pick_card_args &args);
        void handle_action(enums::enum_constant<game_action_type::play_card>, player *p, const play_card_args &args);
        void handle_action(enums::enum_constant<game_action_type::respond_card>, player *p, const play_card_args &args);
        void handle_action(enums::enum_constant<game_action_type::pass_turn>, player *p);
        void handle_action(enums::enum_constant<game_action_type::resolve>, player *p);
    };

}

#endif