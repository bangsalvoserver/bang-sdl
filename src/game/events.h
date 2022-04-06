#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <map>
#include <functional>

#include "utils/enum_variant.h"

#include "card_enums.h"
#include "player.h"

namespace banggame {

    struct request_bang;

    #define EVENT(name, ...) (name, std::function<void(__VA_ARGS__)>)
    
    DEFINE_ENUM_TYPES(event_type,
        (delayed_action)
        EVENT(on_discard_pass,                  player *origin, card *target_card)
        EVENT(on_draw_check,                    player *origin, card *target_card)
        EVENT(trigger_tumbleweed,               card *origin_card, card *drawn_card)
        EVENT(apply_suit_modifier,              card_suit_type &)
        EVENT(apply_value_modifier,             card_value_type &)
        EVENT(apply_beer_modifier,              player *origin, int &value)
        EVENT(apply_maxcards_modifier,          player *origin, int &value)
        EVENT(apply_maxcards_adder,             player *origin, int &value)
        EVENT(apply_volcanic_modifier,          player *origin, bool &value)
        EVENT(apply_immunity_modifier,          card *origin_card, player *target, bool &value)
        EVENT(apply_escapable_modifier,         card *origin_card, player *origin, const player *target, effect_flags flags, bool &value)
        EVENT(apply_initial_cards_modifier,     player *origin, int &value)
        EVENT(apply_chosen_card_modifier,       player *origin, card* &target_card)
        EVENT(apply_bang_modifier,              player *origin, request_bang *req)
        EVENT(verify_target_unique,             card *origin_card, player *origin, player *target)
        EVENT(on_discard_card,                  player *origin, player *target, card *target_card)
        EVENT(on_hit,                           card *origin_card, player *origin, player *target, int damage, bool is_bang)
        EVENT(on_missed,                        card *origin_card, player *origin, player *target, bool is_bang)
        EVENT(on_player_death,                  player *origin, player *target)
        EVENT(on_equip,                         player *origin, player *target, card *target_card)
        EVENT(on_play_hand_card,                player *origin, card *target_card)
        EVENT(post_discard_card,                player *target, card *target_card)
        EVENT(post_discard_orange_card,         player *target, card *target_card)
        EVENT(on_effect_end,                    player *origin, card *target_card)
        EVENT(on_card_drawn,                    player *origin, card *target_card)
        EVENT(on_play_beer,                     player *origin)
        EVENT(pre_turn_start,                   player *origin)
        EVENT(before_turn_start,                player *origin)
        EVENT(on_turn_start,                    player *origin)
        EVENT(on_turn_end,                      player *origin)
        EVENT(post_turn_end,                    player *origin)
        EVENT(on_request_draw,                  player *origin)
        EVENT(on_draw_from_deck,                player *origin)
        EVENT(post_draw_cards,                  player *origin)
    )

    using event_function = enums::enum_variant<event_type>;

    namespace detail {
        template<typename Function> struct function_unwrapper{};
        template<typename ... Args> struct function_unwrapper<std::function<void(Args ...)>> {
            using type = std::tuple<Args...>;
        };

        template<event_type E> struct unwrap_enum_type_function {
            using type = typename function_unwrapper<enums::enum_type_t<E>>::type;
        };

        template<> struct unwrap_enum_type_function<event_type::delayed_action> {
            using type = std::function<void()>;
        };

        template<typename ESeq> struct make_function_argument_tuple_variant{};
        template<enums::reflected_enum auto ... Es> struct make_function_argument_tuple_variant<enums::enum_sequence<Es...>> {
            using type = std::variant<typename unwrap_enum_type_function<Es>::type ...>;
        };

        template<enums::reflected_enum E> using function_argument_tuple_variant = typename make_function_argument_tuple_variant<enums::make_enum_sequence<E>>::type;
    };

    using event_args = detail::function_argument_tuple_variant<event_type>;

    struct event_card_key {
        int card_id;
        int priority;

        event_card_key(card *target_card, int priority = 0)
            : card_id(target_card->id), priority(priority) {}

        auto operator <=> (const event_card_key &rhs) const {
            return priority == rhs.priority
                ? card_id <=> rhs.card_id
                : rhs.priority <=> priority;
        }

        bool operator == (const event_card_key &rhs) const = default;
    };

    struct event_card_key_compare {
        using is_transparent = void;

        bool operator ()(const event_card_key &lhs, card *rhs) const {
            return lhs.card_id < rhs->id;
        }

        bool operator ()(card *lhs, const event_card_key &rhs) const {
            return lhs->id < rhs.card_id;
        }

        bool operator()(const event_card_key &lhs, const event_card_key &rhs) const {
            return lhs < rhs;
        }
    };

    template<typename T>
    struct card_multimap : std::multimap<event_card_key, T, event_card_key_compare> {
        using base = std::multimap<event_card_key, T, event_card_key_compare>;

        template<typename ... Ts>
        void add(event_card_key key, Ts && ... args) {
            base::emplace(std::piecewise_construct, std::make_tuple(key), std::make_tuple(std::forward<Ts>(args) ... ));
        }

        void erase(card *target_card) {
            auto [low, high] = base::equal_range(target_card);
            base::erase(low, high);
        }

        void erase(event_card_key key) {
            base::erase(key);
        }
    };

    struct event_handler_map {
        template<typename T>
        struct single_call_event {
            event_handler_map &parent;
            event_card_key key;
            T function;

            template<typename ... Ts>
            void operator()(Ts && ... args) {
                if (std::invoke(function, std::forward<Ts>(args) ... )) {
                    parent.remove_events(key);
                }
            }
        };

        card_multimap<event_function> m_event_handlers;

        template<event_type E, typename Function>
        void add_event(event_card_key key, Function &&fun) {
            m_event_handlers.add(key, enums::enum_tag<E>, std::forward<Function>(fun));
        }

        template<event_type E, typename Function>
        void add_single_call_event(event_card_key key, Function &&fun) {
            add_event<E>(key, single_call_event{*this, key, std::forward<Function>(fun)});
        }

        void remove_events(auto key) {
            m_event_handlers.erase(key);
        }

        template<event_type E, typename ... Ts>
        void instant_event(Ts && ... args) {
            handle_event(event_args{std::in_place_index<enums::indexof(E)>, std::forward<Ts>(args) ...});
        }
        
        void handle_event(event_args &&event);
    };

}

#endif