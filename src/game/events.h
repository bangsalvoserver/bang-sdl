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
        EVENT(apply_sign_modifier,              player *origin, card_sign &value)
        EVENT(apply_beer_modifier,              player *origin, int &value)
        EVENT(apply_maxcards_modifier,          player *origin, int &value)
        EVENT(apply_volcanic_modifier,          player *origin, bool &value)
        EVENT(apply_immunity_modifier,          card *origin_card, player *target, bool &value)
        EVENT(apply_escapable_modifier,         card *origin_card, player *origin, const player *target, effect_flags flags, bool &value)
        EVENT(apply_initial_cards_modifier,     player *origin, int &value)
        EVENT(apply_chosen_card_modifier,       player *origin, card* &target_card)
        EVENT(apply_bang_modifier,              player *origin, request_bang *req)

        EVENT(verify_target_unique,             card *origin_card, player *origin, player *target, bool &value)
        
        // viene chiamato quando scarti una carta a fine turno
        EVENT(on_discard_pass, player *origin, card *target_card)

        // viene chiamata quando estrai una carta nel momento che viene pescata
        EVENT(on_draw_check, player *origin, card *target_card)

        // viene chiamato quando estrai una carta nel momento che viene scelta
        EVENT(on_draw_check_select, player *origin, card *origin_card, card *drawn_card)

        // viene chiamato quando si scarta VOLONTARIAMENTE una carta (si gioca cat balou o panico contro una carta)
        EVENT(on_discard_card, player *origin, player *target, card *target_card)

        // viene chiamato quando una carta arancione viene scartata perche' sono finiti i cubetti
        EVENT(on_discard_orange_card, player *target, card *target_card)

        // viene chiamato quando un giocatore viene colpito
        EVENT(on_hit, card *origin_card, player *origin, player *target, int damage, bool is_bang)

        // viene chiamato quando un giocatore gioca mancato
        EVENT(on_missed, card *origin_card, player *origin, player *target, bool is_bang)

        // viene chiamato quando un giocatore muore
        EVENT(on_player_death, player *origin, player *target)

        // viene chiamato quando un giocatore equipaggia una carta
        EVENT(on_equip_card, player *origin, player *target, card *target_card)

        // viene chiamato quando un giocatore gioca una carta dalla mano
        EVENT(on_play_hand_card, player *origin, card *target_card)

        // viene chiamato dopo la fine di un effetto
        EVENT(on_effect_end, player *origin, card *target_card)

        // viene chiamato quando un giocatore gioca birra
        EVENT(on_play_beer, player *origin)

        // viene chiamato prima dell'inizio del turno, prima delle estrazioni
        EVENT(pre_turn_start, player *origin)

        // viene chiamato all'inizio del turno, prima di pescare
        EVENT(on_turn_start, player *origin)

        // viene chiamato all'inizio del turno, prima di attivare fase di pesca
        EVENT(on_request_draw, player *origin)

        // viene chiamato quando si clicca sul mazzo per pescare in fase di pesca
        EVENT(on_draw_from_deck, player *origin)

        // viene chiamato quando si pesca una carta in fase di pesca
        EVENT(on_card_drawn, player *origin, card *target_card)

        // viene chiamato dopo che un giocatore finisce la fase di pesca
        EVENT(post_draw_cards, player *origin)
        
        // viene chiamato alla fine del turno
        EVENT(on_turn_end, player *origin)

        // viene chiamato dopo la fine del turno, prima dei turni extra
        EVENT(post_turn_end, player *origin)
    )

    using event_function = enums::enum_variant<event_type>;

    struct event_card_key {
        int card_id;
        int priority;

        event_card_key(card *target_card, int priority = 0)
            : card_id(target_card->id), priority(priority) {}

        auto operator <=> (const event_card_key &other) const = default;

        auto operator <=> (card *other) const {
            return card_id <=> other->id;
        }
    };

    template<typename T>
    struct card_multimap : std::multimap<event_card_key, T, std::less<>> {
        using base = std::multimap<event_card_key, T, std::less<>>;

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

    template<typename T, typename U> struct same_function_args {};

    template<typename Function, typename RetType, typename ... Ts>
    struct same_function_args<Function, std::function<RetType(Ts...)>> : std::is_invocable_r<RetType, Function, Ts...> {};

    template<typename T, event_type E>
    concept invocable_for_event = same_function_args<T, enums::enum_type_t<E>>::value;

    struct event_handler_map {
        card_multimap<event_function> m_event_handlers;

        template<event_type E, invocable_for_event<E> Function>
        void add_event(event_card_key key, Function &&fun) {
            m_event_handlers.add(key, enums::enum_tag<E>, std::forward<Function>(fun));
        }

        void remove_events(auto key) {
            m_event_handlers.erase(key);
        }

        template<event_type E, typename ... Ts>
        void call_event(Ts && ... args) {
            using vector_pair = std::pair<int, enums::enum_type_t<E> *>;
            std::vector<vector_pair> handlers;

            for (auto &[key, handler] : m_event_handlers) {
                if (auto *fun = std::get_if<enums::indexof(E)>(&handler)) {
                    handlers.emplace_back(key.priority, fun);
                }
            }

            std::ranges::sort(handlers, std::greater(), &vector_pair::first);

            for (auto &[key, fun] : handlers) {
                std::invoke(*fun, args ...);
            }
        }
    };

}

#endif