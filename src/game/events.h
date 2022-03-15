#ifndef __EVENTS_H__
#define __EVENTS_H__

#include <map>
#include <functional>

#include "utils/enum_variant.h"

#include "card_enums.h"

namespace banggame {

    struct card;
    struct player;
    struct request_bang;
    
    DEFINE_ENUM_TYPES_IN_NS(banggame, event_type,
        (delayed_action,    std::function<void(std::function<void()>)>)
        (on_discard_pass,   std::function<void(player *origin, card *target_card)>)
        (on_draw_check,     std::function<void(player *origin, card *target_card)>)
        (trigger_tumbleweed, std::function<void(card *origin_card, card *drawn_card)>)
        (apply_suit_modifier, std::function<void(card_suit_type &)>)
        (apply_value_modifier, std::function<void(card_value_type &)>)
        (apply_beer_modifier, std::function<void(player *origin, int &value)>)
        (apply_maxcards_modifier, std::function<void(player *origin, int &value)>)
        (apply_maxcards_adder, std::function<void(player *origin, int &value)>)
        (apply_volcanic_modifier, std::function<void(player *origin, bool &value)>)
        (apply_immunity_modifier, std::function<void(card *origin_card, player *target, bool &value)>)
        (apply_escapable_modifier, std::function<void(card *origin_card, player *origin, const player *target, effect_flags flags, bool &value)>)
        (apply_initial_cards_modifier, std::function<void(player *origin, int &value)>)
        (apply_chosen_card_modifier, std::function<void(player *origin, card* &target_card)>)
        (apply_bang_modifier, std::function<void(player *origin, request_bang *req)>)
        (verify_target_unique, std::function<void(card *origin_card, player *origin, player *target)>)
        (on_discard_card,   std::function<void(player *origin, player *target, card *target_card)>)
        (on_hit,            std::function<void(card *origin_card, player *origin, player *target, int damage, bool is_bang)>)
        (on_missed,         std::function<void(card *origin_card, player *origin, player *target, bool is_bang)>)
        (on_player_death,   std::function<void(player *origin, player *target)>)
        (on_equip,          std::function<void(player *origin, player *target, card *target_card)>)
        (on_play_hand_card, std::function<void(player *origin, card *target_card)>)
        (post_discard_card, std::function<void(player *target, card *target_card)>)
        (post_discard_orange_card, std::function<void(player *target, card *target_card)>)
        (on_effect_end,     std::function<void(player *origin, card *target_card)>)
        (on_card_drawn,     std::function<void(player *origin, card *target_card)>)
        (on_play_bang,      std::function<void(player *origin)>)
        (on_play_beer,      std::function<void(player *origin)>)
        (pre_turn_start,    std::function<void(player *origin)>)
        (before_turn_start, std::function<void(player *origin)>)
        (on_turn_start,     std::function<void(player *origin)>)
        (on_turn_end,       std::function<void(player *origin)>)
        (post_turn_end,     std::function<void(player *origin)>)
        (on_request_draw,   std::function<void(player *origin)>)
        (on_draw_from_deck, std::function<void(player *origin)>)
        (post_draw_cards,   std::function<void(player *origin)>)
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

    struct event_handler_map {
        std::multimap<card *, event_function> m_event_handlers;

        template<event_type E, typename Function>
        void add_event(card *target_card, Function &&fun) {
            m_event_handlers.emplace(std::piecewise_construct,
                std::make_tuple(target_card),
                std::make_tuple(enums::enum_constant<E>{}, std::forward<Function>(fun)));
        }

        void remove_events(card *target_card) {
            m_event_handlers.erase(target_card);
        }

        template<event_type E, typename ... Ts>
        void instant_event(Ts && ... args) {
            event_args event{std::in_place_index<enums::indexof(E)>, std::forward<Ts>(args) ...};
            handle_event(event);
        }
        
        void handle_event(event_args &event);
    };

}

#endif