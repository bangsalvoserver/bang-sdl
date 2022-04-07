#ifndef __GAME_H__
#define __GAME_H__

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

    #define ACTION_TAG(name) enums::enum_tag_t<game_action_type::name>
    
    DEFINE_ENUM_FLAGS(scenario_flags,
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

        std::deque<request_holder> m_requests;
        struct draw_check_handler {
            draw_check_function function;
            player *origin = nullptr;
            card *origin_card = nullptr;
        };
        std::optional<draw_check_handler> m_current_check;
        
        std::deque<std::function<void()>> m_delayed_actions;

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

        card_multimap<card_disabler_fun> m_disablers;

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

        player *find_disconnected_player();

        std::default_random_engine rng;

        void shuffle_cards_and_ids(std::vector<card *> &vec);

        template<game_update_type E, typename ... Ts>
        void add_private_update(player *p, Ts && ... args) {
            m_updates.emplace_back(std::piecewise_construct,
                std::make_tuple(p),
                std::make_tuple(enums::enum_tag<E>, std::forward<Ts>(args) ...));
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

        template<typename T = request_base>
        T *top_request_if(player *target = nullptr) {
            if (m_requests.empty()) return nullptr;
            auto &req = top_request();
            return !target || req.target() == target ? req.get_if<T>() : nullptr;
        }

        template<typename T>
        bool top_request_is(player *target = nullptr) {
            return top_request_if<T>(target) != nullptr;
        }
        
        request_status_args make_request_update(player *p);
        void send_request_update();

        void add_request(std::shared_ptr<request_base> &&value);
        void queue_request(std::shared_ptr<request_base> &&value);

        template<std::derived_from<request_base> T, typename ... Ts>
        void add_request(Ts && ... args) {
            add_request(std::make_shared<T>(std::forward<Ts>(args) ... ));
        }
        
        template<std::derived_from<request_base> T, typename ... Ts>
        void queue_request(Ts && ... args) {
            queue_request(std::make_shared<T>(std::forward<Ts>(args) ... ));
        }

        template<typename T = request_base>
        bool pop_request_noupdate() {
            if (!top_request_is<T>()) return false;
            m_requests.pop_front();
            return true;
        }

        template<typename T = request_base>
        bool pop_request() {
            return pop_request_noupdate<T>() && (flush_actions(), true);
        }

        void tick();

        void queue_action(std::function<void()> &&fun);
        void flush_actions();

        template<event_type E, typename ... Ts>
        void queue_event(Ts && ... args) {
            queue_action([this, ...args = std::forward<Ts>(args)] () mutable {
                call_event<E>(std::forward<Ts>(args) ... );
            });
        }

        std::vector<card *> &get_pile(card_pile_type pile, player *owner = nullptr);
        std::vector<card *>::iterator move_to(card *c, card_pile_type pile, bool known = true, player *owner = nullptr, show_card_flags flags = {});
        card *draw_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = {});
        card *draw_phase_one_card_to(card_pile_type pile, player *owner = nullptr, show_card_flags flags = {});

        card *draw_shop_card();
        
        void draw_scenario_card();

        void draw_check_then(player *origin, card *origin_card, draw_check_function fun);
        void do_draw_check();

        void add_disabler(event_card_key key, card_disabler_fun &&fun);
        void remove_disablers(event_card_key key);
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

    inline std::vector<card_backface> make_id_vector(auto &&range) {
        auto view = range | std::views::transform([](const card *c) {
            return card_backface{c->id, c->deck};
        });
        return {view.begin(), view.end()};
    };

}

#endif