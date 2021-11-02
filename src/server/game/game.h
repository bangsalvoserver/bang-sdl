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
#include "common/requests.h"
#include "common/timer.h"

namespace banggame {

    struct game_options {
        int nplayers = 0;
        card_expansion_type expansions = enums::flags_all<card_expansion_type>;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, event_type,
        (delayed_action,    std::function<void(std::function<void()>)>)
        (on_discard_pass,   std::function<void(player *origin, int card_id)>)
        (on_draw_check,     std::function<void(int card_id)>)
        (on_discard_card,   std::function<void(player *origin, player *target, int card_id)>)
        (on_hit,            std::function<void(player *origin, player *target, int damage, bool is_bang)>)
        (on_missed,         std::function<void(player *origin, player *target, bool is_bang)>)
        (on_player_death,   std::function<void(player *origin, player *target)>)
        (on_equip,          std::function<void(player *origin, player *target, int card_id)>)
        (on_play_hand_card, std::function<void(player *origin, int card_id)>)
        (post_discard_card, std::function<void(player *target, int card_id)>)
        (post_discard_orange_card, std::function<void(player *target, int card_id)>)
        (on_effect_end,     std::function<void(player *origin, int card_id)>)
        (on_play_bang,      std::function<void(player *origin)>)
        (on_play_beer,      std::function<void(player *origin)>)
        (on_turn_start,     std::function<void(player *origin)>)
        (on_turn_end,       std::function<void(player *origin)>)
        (on_draw_from_deck, std::function<void(player *origin)>)
        (on_game_start,     std::function<void()>)
    )

    using event_function = enums::enum_variant<event_type>;

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

    struct game_error : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    struct game_log {
        player *origin;
        player *target;
        std::string message;
    };

    #define ACTION_TAG(name) enums::enum_constant<game_action_type::name>

    struct game {
        std::list<std::pair<player *, game_update>> m_updates;
        std::list<game_log> m_logs;

        std::list<request_holder> m_requests;
        std::list<draw_check_function> m_pending_checks;
        std::multimap<int, event_function> m_event_handlers;
        std::list<event_args> m_pending_events;
        
        std::vector<deck_card> m_deck;
        std::vector<deck_card> m_discards;
        std::vector<deck_card> m_selection;
        std::vector<player> m_players;

        std::vector<deck_card> m_shop_deck;
        std::vector<deck_card> m_shop_discards;
        std::vector<deck_card> m_shop_hidden;
        std::vector<deck_card> m_shop_selection;

        std::vector<int> m_cubes;

        std::vector<character> m_base_characters;

        using table_card_disabler = std::pair<int, bool>;
        std::vector<table_card_disabler> m_table_card_disablers;

        player *m_playing = nullptr;
        player *m_next_turn = nullptr;

        game_options m_options;

        int m_id_counter = 0;
        
        int get_next_id() {
            return ++m_id_counter;
        }

        std::default_random_engine rng;

        template<game_update_type E, typename ... Ts>
        void add_private_update(player *p, Ts && ... args) {
            m_updates.emplace_back(std::piecewise_construct,
                std::make_tuple(p),
                std::make_tuple(enums::enum_constant<E>{}, std::forward<Ts>(args) ...));
        }

        template<game_update_type E, typename ... Ts>
        void add_public_update(const Ts & ... args) {
            add_private_update<E>(nullptr, args ...);
        }

        void add_log(player *origin, player *target, std::string message) {
            m_logs.emplace_back(origin, target, std::move(message));
        }

        std::vector<game_update> get_game_state_updates();

        void send_card_update(const deck_card &c, player *owner = nullptr, show_card_flags flags = enums::flags_none<show_card_flags>);
        void send_character_update(const character &c, int player_id, int index);

        void start_game(const game_options &options);

        bool has_expansion(card_expansion_type type) const {
            using namespace enums::flag_operators;
            return bool(m_options.expansions & type);
        }

        request_holder &top_request() {
            return m_requests.front();
        }

        bool top_request_is(request_type type, player *target = nullptr) {
            if (m_requests.empty()) return false;
            const auto &req = top_request();
            return req.is(type) && (!target || req.target() == target);
        }

        void send_request_update() {
            const auto &req = top_request();
            add_public_update<game_update_type::request_handle>(req.enum_index(),
                req.origin() ? req.origin()->id : 0,
                req.target() ? req.target()->id : 0);
        }

        template<request_type E>
        auto &add_request(int origin_card_id, player *origin, player *target, bool escapable = false) {
            auto &ret = m_requests.emplace_front(enums::enum_constant<E>{}, origin_card_id, origin, target).template get<E>();
            ret.escapable = escapable;

            send_request_update();

            return ret;
        }

        void queue_request(auto &&req) {
            auto &ret = m_requests.emplace_back(std::move(req));

            if (m_requests.size() == 1) {
                send_request_update();
            }
        }

        template<request_type E>
        auto &queue_request(int origin_card_id, player *origin, player *target, bool escapable = false) {
            auto &ret = m_requests.emplace_back(enums::enum_constant<E>{}, origin_card_id, origin, target).template get<E>();
            ret.escapable = escapable;

            if (m_requests.size() == 1) {
                send_request_update();
            }

            return ret;
        }

        void pop_request() {
            pop_request_noupdate();
            if (m_requests.empty()) {
                add_public_update<game_update_type::status_clear>();
                pop_events();
            } else {
                send_request_update();
            }
        }

        void pop_request_noupdate();

        void tick();

        template<event_type E, typename Function>
        void add_event(int card_id, Function &&fun) {
            m_event_handlers.emplace(std::piecewise_construct,
                std::make_tuple(card_id),
                std::make_tuple(enums::enum_constant<E>{}, std::forward<Function>(fun)));
        }

        void remove_events(int card_id) {
            m_event_handlers.erase(card_id);
        }
        
        void handle_event(event_args &event);

        template<event_type E, typename ... Ts>
        void instant_event(Ts && ... args) {
            event_args event{std::in_place_index<enums::indexof(E)>, std::forward<Ts>(args) ...};
            handle_event(event);
        }

        template<event_type E, typename ... Ts>
        void queue_event(Ts && ... args) {
            event_args event{std::in_place_index<enums::indexof(E)>, std::forward<Ts>(args) ...};
            if (m_requests.empty()) {
                handle_event(event);
            } else {
                m_pending_events.emplace_back(std::move(event));
            }
        }

        void pop_events() {
            while (m_requests.empty() && !m_pending_events.empty()) {
                handle_event(m_pending_events.front());
                m_pending_events.pop_front();
            }
        }

        deck_card &move_to(deck_card &&c, card_pile_type pile, bool known = true, player *owner = nullptr, show_card_flags flags = enums::flags_none<show_card_flags>);
        deck_card &draw_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = enums::flags_none<show_card_flags>);

        deck_card draw_from_discards();
        deck_card draw_from_temp(int card_id);

        deck_card &draw_shop_card();

        void draw_check_then(player *p, draw_check_function fun, bool force_one = false, bool invert_pop_req = false);
        void resolve_check(int card_id);

        void disable_table_cards(int player_id, bool disable_characters = false);
        void enable_table_cards(int player_id);

        bool table_cards_disabled(int player_id);
        bool characters_disabled(int player_id);

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
            return std::min(d1, d2) + to->m_distance_mod;
        }

        void check_game_over(player *target, bool discarded_ghost = false);
        void player_death(player *target);

        void handle_action(ACTION_TAG(pick_card), player *p, const pick_card_args &args);
        void handle_action(ACTION_TAG(play_card), player *p, const play_card_args &args);
        void handle_action(ACTION_TAG(draw_from_deck), player *p);
        void handle_action(ACTION_TAG(pass_turn), player *p);
        void handle_action(ACTION_TAG(resolve), player *p);
    };

}

#endif