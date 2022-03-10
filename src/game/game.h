#ifndef __GAME_H__
#define __GAME_H__

#include <list>
#include <deque>
#include <vector>
#include <algorithm>
#include <functional>
#include <optional>
#include <random>

#include "make_all_cards.h"
#include "player.h"
#include "events.h"

#include "server/net_enums.h"
#include "holders.h"
#include "formatter.h"

#include "utils/id_map.h"

namespace banggame {

    struct game_options {
        card_expansion_type expansions = enums::flags_all<card_expansion_type>;
    };

    struct game_error : std::exception, game_formatted_string {
        using game_formatted_string::game_formatted_string;

        const char *what() const noexcept override {
            return format_str.c_str();
        }
    };

    #define ACTION_TAG(name) enums::enum_constant<game_action_type::name>
    
    DEFINE_ENUM_FLAGS_IN_NS(banggame, scenario_flags,
        (invert_rotation) // inverti giro
        (ghosttown) // citta' fantasma
        (judge) // non si puo' equipaggiare
        (abandonedmine) // fase 1 : pesca dagli scarti, fase 3 : scarta coperto nel mazzo
        (deadman) // il primo morto ritorna in vita con 2 carte e 2 hp nel suo turno
    )

    using card_disabler_fun = std::function<bool(card *)>;

    struct game : event_handler_map {
        std::deque<std::pair<player *, game_update>> m_updates;

        std::deque<game_formatted_string> m_saved_log;

        std::list<request_holder> m_requests;
        struct draw_check_handler {
            draw_check_function function;
            player *origin = nullptr;
            card *origin_card = nullptr;
        };
        std::optional<draw_check_handler> m_current_check;
        
        std::list<event_args> m_pending_events;

        util::id_map<card> m_cards;
        util::id_map<player> m_players;
        
        std::vector<card *> m_deck;
        std::vector<card *> m_discards;
        std::vector<card *> m_selection;

        std::vector<card *> m_shop_deck;
        std::vector<card *> m_shop_discards;
        std::vector<card *> m_hidden_deck;
        std::vector<card *> m_shop_selection;
        std::vector<card *> m_specials;

        std::vector<card *> m_scenario_deck;
        std::vector<card *> m_scenario_cards;
        scenario_flags m_scenario_flags{};

        std::vector<int> m_cubes;

        std::multimap<card *, card_disabler_fun> m_disablers;

        player *m_playing = nullptr;
        player *m_first_player = nullptr;
        player *m_first_dead = nullptr;

        game_options m_options;

        bool m_game_over = false;

        game() {
            std::random_device rd;
            rng.seed(rd());
        }

        card *find_card(int card_id);
        player *find_player(int player_id);

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

        template<typename ... Ts>
        void add_log(Ts && ... args) {
            add_public_update<game_update_type::game_log>(m_saved_log.emplace_back(std::forward<Ts>(args) ...));
        }

        std::vector<game_update> get_game_state_updates(player *owner);

        void send_card_update(const card &c, player *owner = nullptr, show_card_flags flags = {});

        void start_game(const game_options &options, const all_cards_t &all_cards);

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

        request_status_args make_request_update(player *p);
        void send_request_update();

        void add_request(auto &&req) {
            auto &ret = m_requests.emplace_front(std::move(req));
            send_request_update();
        }

        template<request_type E, typename ... Ts>
        void add_request(Ts && ... args) {
            auto &ret = m_requests.emplace_front(enums::enum_constant<E>{}, std::forward<Ts>(args) ...).template get<E>();
            send_request_update();
        }

        void queue_request(auto &&req) {
            auto &ret = m_requests.emplace_back(std::move(req));

            if (m_requests.size() == 1) {
                send_request_update();
            }
        }

        template<request_type E, typename ... Ts>
        void queue_request(Ts && ... args) {
            auto &ret = m_requests.emplace_back(enums::enum_constant<E>{}, std::forward<Ts>(args) ...);

            if (m_requests.size() == 1) {
                send_request_update();
            }
        }

        bool pop_request_noupdate(request_type type = request_type::none);
        bool pop_request(request_type type = request_type::none);
        void events_after_requests();

        void tick();

        template<event_type E, typename ... Ts>
        void queue_event(Ts && ... args) {
            event_args event{std::in_place_index<enums::indexof(E)>, std::forward<Ts>(args) ...};
            if (m_requests.empty()) {
                handle_event(event);
            } else {
                m_pending_events.emplace_back(std::move(event));
            }
        }

        void queue_delayed_action(auto &&fun) {
            if (m_requests.empty()) {
                fun();
            } else {
                queue_event<event_type::delayed_action>(std::forward<decltype(fun)>(fun));
            }
        }

        void pop_events() {
            while (m_requests.empty() && !m_pending_events.empty()) {
                handle_event(m_pending_events.front());
                m_pending_events.pop_front();
            }
        }

        std::vector<card *> &get_pile(card_pile_type pile, player *owner = nullptr);
        std::vector<card *>::iterator move_to(card *c, card_pile_type pile, bool known = true, player *owner = nullptr, show_card_flags flags = {});
        card *draw_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = {});
        card *draw_phase_one_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = {});

        card *draw_shop_card();
        
        void draw_scenario_card();

        void draw_check_then(player *origin, card *origin_card, draw_check_function fun);
        void do_draw_check();

        void add_disabler(card *target_card, card_disabler_fun &&fun);
        void remove_disablers(card *target_card);
        bool is_disabled(card *target_card) const;

        int num_alive() const {
            return std::ranges::count_if(m_players, &player::alive);
        }

        player *get_next_player(player *p);
        player *get_next_in_turn(player *p);

        int calc_distance(player *from, player *to);

        void check_game_over(player *killer, player *target);
        void player_death(player *killer, player *target);

        void handle_action(ACTION_TAG(pick_card), player *p, const pick_card_args &args);
        void handle_action(ACTION_TAG(play_card), player *p, const play_card_args &args);
        void handle_action(ACTION_TAG(respond_card), player *p, const play_card_args &args);
    };

    inline std::vector<int> make_id_vector(auto &&range) {
        auto view = range | std::views::transform(&card::id);
        return {view.begin(), view.end()};
    };

}

#endif