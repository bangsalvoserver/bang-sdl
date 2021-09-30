#ifndef __GAME_H__
#define __GAME_H__

#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <random>

#include "card.h"
#include "player.h"

#include "common/game_update.h"
#include "common/responses.h"

namespace banggame {

    using draw_check_function = std::function<void(card_suit_type, card_value_type)>;

    struct game_options {
        int nplayers = 0;
        card_expansion_type allowed_expansions = enums::flags_all<card_expansion_type>;
    };

    namespace detail {
        template<typename Function> struct function_unwrapper{};
        template<typename Function> using function_unwrapper_t = typename function_unwrapper<Function>::type;
        template<typename ... Args> struct function_unwrapper<std::function<void(Args ...)>> {
            using type = std::tuple<Args...>;
        };

        template<typename ESeq> struct make_function_argument_tuple_variant{};
        template<enums::reflected_enum auto ... Es> struct make_function_argument_tuple_variant<enums::enum_sequence<Es...>> {
            using type = std::variant<function_unwrapper_t<enums::enum_type_t<Es>> ...>;
        };

        template<enums::reflected_enum E> using function_argument_tuple_variant = typename make_function_argument_tuple_variant<enums::make_enum_sequence<E>>::type;
    };

    using event_args = detail::function_argument_tuple_variant<event_type>;

    struct game {
        std::list<std::pair<player *, game_update>> m_updates;
        std::list<std::pair<response_type, response_holder>> m_responses;
        std::list<draw_check_function> m_pending_checks;
        std::map<int, event_function> m_event_handlers;
        std::list<event_args> m_pending_events;
        
        std::vector<deck_card> m_deck;
        std::vector<deck_card> m_discards;
        std::vector<deck_card> m_temps;
        std::vector<player> m_players;

        player *m_playing = nullptr;

        int m_id_counter = 0;
        
        int get_next_id() {
            return ++m_id_counter;
        }

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

        void add_show_card(const deck_card &c, player *owner = nullptr);

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
                pop_events();
                add_public_update<game_update_type::response_done>();
            } else {
                add_response_update();
            }
        }

        void pop_response_noupdate() {
            m_responses.pop_front();
        }

        template<event_type E, typename Function>
        void add_event(int card_id, Function &&fun) {
            m_event_handlers.try_emplace(card_id, enums::enum_constant<E>{}, std::forward<Function>(fun));
        }

        void remove_event(int card_id) {
            m_event_handlers.erase(card_id);
        }

        template<event_type E, typename ... Ts>
        void handle_game_event(Ts && ... args) {
            m_pending_events.emplace_back(std::in_place_index<enums::indexof(E)>, std::forward<Ts>(args) ...);
            if (m_responses.empty()) {
                pop_events();
            }
        }
        
        void pop_events();

        deck_card &add_to_discards(deck_card &&c) {
            add_show_card(c);
            add_public_update<game_update_type::move_card>(c.id, 0, card_pile_type::discard_pile);
            return m_discards.emplace_back(std::move(c));
        }

        deck_card &add_to_temps(deck_card &&c, player *owner = nullptr) {
            add_show_card(c, owner);
            add_public_update<game_update_type::move_card>(c.id, 0, card_pile_type::temp_table);
            return m_temps.emplace_back(std::move(c));
        }

        deck_card draw_card();
        deck_card draw_from_discards();
        deck_card draw_from_temp(int card_id);

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
            return std::min(d1, d2) + to->m_distance_mod - from->m_range_mod;
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
        void handle_action(enums::enum_constant<game_action_type::draw_from_deck>, player *p);
        void handle_action(enums::enum_constant<game_action_type::pass_turn>, player *p);
        void handle_action(enums::enum_constant<game_action_type::resolve>, player *p);
    };

}

#endif