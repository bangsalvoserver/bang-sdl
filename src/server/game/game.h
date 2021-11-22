#ifndef __GAME_H__
#define __GAME_H__

#include <list>
#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <random>

#include "card.h"
#include "player.h"

#include "common/game_update.h"
#include "common/requests.h"
#include "common/timer.h"

namespace banggame {

    struct game_options {
        card_expansion_type expansions = enums::flags_all<card_expansion_type>;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, event_type,
        (delayed_action,    std::function<void(std::function<void()>)>)
        (on_discard_pass,   std::function<void(player *origin, card *target_card)>)
        (on_draw_check,     std::function<void(player *origin, card *target_card)>)
        (trigger_tumbleweed, std::function<void(card *drawn_card)>)
        (apply_suit_modifier, std::function<void(card_suit_type &)>)
        (apply_value_modifier, std::function<void(card_value_type &)>)
        (on_discard_card,   std::function<void(player *origin, player *target, card *target_card)>)
        (on_hit,            std::function<void(card *origin_card, player *origin, player *target, int damage, bool is_bang)>)
        (on_missed,         std::function<void(card *origin_card, player *origin, player *target, bool is_bang)>)
        (on_player_death,   std::function<void(player *origin, player *target)>)
        (on_equip,          std::function<void(player *origin, player *target, card *target_card)>)
        (on_play_hand_card, std::function<void(player *origin, card *target_card)>)
        (post_discard_card, std::function<void(player *target, card *target_card)>)
        (post_discard_orange_card, std::function<void(player *target, card *target_card)>)
        (on_play_card_end,  std::function<void(player *origin, card *target_card)>)
        (on_effect_end,     std::function<void(player *origin, card *target_card)>)
        (on_card_drawn,     std::function<void(player *origin, card *target_card)>)
        (on_play_bang,      std::function<void(player *origin)>)
        (on_play_beer,      std::function<void(player *origin)>)
        (pre_turn_start,    std::function<void(player *origin)>)
        (on_turn_start,     std::function<void(player *origin)>)
        (on_turn_end,       std::function<void(player *origin)>)
        (on_draw_from_deck, std::function<void(player *origin)>)
        (post_draw_cards,   std::function<void(player *origin)>)
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
        struct draw_check_handler {
            draw_check_function function;
            player *origin = nullptr;
            bool no_auto_resolve = false;
        };
        std::optional<draw_check_handler> m_current_check;
        std::multimap<card *, event_function> m_event_handlers;
        std::list<event_args> m_pending_events;

        std::map<int, card> m_cards;
        std::map<int, character> m_characters;
        
        std::vector<card *> m_deck;
        std::vector<card *> m_discards;
        std::vector<card *> m_selection;
        std::vector<player> m_players;

        std::vector<card *> m_shop_deck;
        std::vector<card *> m_shop_discards;
        std::vector<card *> m_hidden_deck;
        std::vector<card *> m_shop_selection;

        std::vector<card *> m_scenario_deck;
        std::vector<card *> m_scenario_cards;
        scenario_flags m_scenario_flags = enums::flags_none<scenario_flags>;

        std::vector<int> m_cubes;

        std::vector<character *> m_base_characters;

        int m_table_cards_disabled = 0;
        int m_characters_disabled = 0;

        player *m_playing = nullptr;
        player *m_first_player = nullptr;
        bool m_ignore_next_turn = false;

        game_options m_options;

        int m_id_counter = 0;

        game() {
            std::random_device rd;
            rng.seed(rd());
        }
        
        int get_next_id() {
            return ++m_id_counter;
        }

        card *find_card(int card_id);

        std::default_random_engine rng;

        void shuffle_cards_and_ids(std::vector<card *> &vec);

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

        std::vector<game_update> get_game_state_updates(player *owner);

        void send_card_update(const card &c, player *owner = nullptr, show_card_flags flags = enums::flags_none<show_card_flags>);
        void send_character_update(const character &c, int player_id, int index);

        void start_game(const game_options &options);

        bool has_expansion(card_expansion_type type) const {
            using namespace enums::flag_operators;
            return bool(m_options.expansions & type);
        }

        bool has_scenario(scenario_flags type) const {
            using namespace enums::flag_operators;
            return bool(m_scenario_flags & type);
        }

        request_holder &top_request() {
            return m_requests.front();
        }

        bool top_request_is(request_type type, player *target = nullptr) {
            if (m_requests.empty()) return false;
            const auto &req = top_request();
            return req.is(type) && (!target || req.target() == target);
        }

        void send_request_update();

        template<request_type E>
        auto &add_request(card *origin_card, player *origin, player *target, effect_flags flags = enums::flags_none<effect_flags>) {
            auto &ret = m_requests.emplace_front(enums::enum_constant<E>{}, origin_card, origin, target, flags).template get<E>();

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
        auto &queue_request(card *origin_card, player *origin, player *target, effect_flags flags = enums::flags_none<effect_flags>) {
            auto &ret = m_requests.emplace_back(enums::enum_constant<E>{}, origin_card, origin, target, flags).template get<E>();

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
        void add_event(card *target_card, Function &&fun) {
            m_event_handlers.emplace(std::piecewise_construct,
                std::make_tuple(target_card),
                std::make_tuple(enums::enum_constant<E>{}, std::forward<Function>(fun)));
        }

        void remove_events(card *target_card) {
            m_event_handlers.erase(target_card);
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

        std::vector<card *> &get_pile(card_pile_type pile, player *owner = nullptr);
        std::vector<card *>::iterator move_to(card *c, card_pile_type pile, bool known = true, player *owner = nullptr, show_card_flags flags = enums::flags_none<show_card_flags>);
        card *draw_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = enums::flags_none<show_card_flags>);
        card *draw_phase_one_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = enums::flags_none<show_card_flags>);

        card *draw_shop_card();
        
        void draw_scenario_card();

        void draw_check_then(player *p, draw_check_function fun);
        void do_draw_check(player *p);

        void resolve_check(player *p, card *card);

        void disable_table_cards();
        void enable_table_cards();
        bool table_cards_disabled(player *p) const;

        void disable_characters();
        void enable_characters();
        bool characters_disabled(player *p) const;

        int num_alive() const {
            return std::ranges::count_if(m_players, &player::alive);
        }

        player *get_next_player(player *p);
        player *get_next_in_turn(player *p);

        player *get_player(int id) {
            auto it = std::ranges::find(m_players, id, &player::id);
            if (it != m_players.end()) {
                return &*it;
            } else {
                return nullptr;
            }
        }

        int calc_distance(player *from, player *to);

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