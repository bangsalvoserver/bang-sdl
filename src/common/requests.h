#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include "effect_holder.h"
#include "characters.h"
#include "timer.h"

namespace banggame {

    struct request_predraw : request_base {
        request_predraw(player *target)
            : request_base(nullptr, nullptr, target) {}
        
        game_formatted_string status_text() const;
    };

    struct request_draw : request_base, allowed_piles<card_pile_type::main_deck> {
        request_draw(player *target)
            : request_base(nullptr, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_check : request_base, allowed_piles<card_pile_type::selection> {
        request_check(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_generalstore : request_base, allowed_piles<card_pile_type::selection> {
        request_generalstore(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_discard : request_base, allowed_piles<card_pile_type::player_hand> {
        request_discard(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        int ncards = 1;
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_discard_pass : request_base, allowed_piles<card_pile_type::player_hand> {
        player *next_player;
        
        request_discard_pass(player *target, player *next_player)
            : request_base(nullptr, nullptr, target)
            , next_player(next_player) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_indians : request_base {
        using request_base::request_base;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_duel : request_base {
        request_duel(card *origin_card, player *origin, player *target, player *respond_to, effect_flags flags = no_effect_flags)
            : request_base(origin_card, origin, target, flags)
            , respond_to(respond_to) {}

        player *respond_to = nullptr;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_bang : request_base {
        using request_base::request_base;

        std::vector<card *> barrels_used;
        int bang_strength = 1;
        int bang_damage = 1;
        bool unavoidable = false;
        bool is_bang_card = false;

        std::function<void()> cleanup_function;

        void on_resolve();
        void cleanup();
        game_formatted_string status_text() const;
    };

    struct request_destroy : request_base {
        request_destroy(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags = no_effect_flags)
            : request_base(origin_card, origin, target, flags)
            , target_card(target_card) {}

        card *target_card;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_steal : request_base {
        request_steal(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags = no_effect_flags)
            : request_base(origin_card, origin, target, flags)
            , target_card(target_card) {}
        
        card *target_card;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_death : request_base {
        request_death(card *origin_card, player *origin, player *target)
            : request_base(origin_card, origin, target) {}

        std::vector<card *> draw_attempts;
        
        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_bandidos : request_base, allowed_piles<card_pile_type::player_hand> {
        using request_base::request_base;

        int num_cards = 2;

        void on_pick(card_pile_type pile, player *target, card *target_card);
        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_tornado : request_base, allowed_piles<card_pile_type::player_hand> {
        using request_base::request_base;
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_poker : request_base, allowed_piles<card_pile_type::player_hand> {
        using request_base::request_base;

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_poker_draw : request_base, allowed_piles<card_pile_type::selection> {
        request_poker_draw(card *origin_card, player *origin)
            : request_base(origin_card, nullptr, target) {}

        int num_cards = 2;

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_saved : request_base, allowed_piles<card_pile_type::player_hand, card_pile_type::main_deck> {
        request_saved(card *origin_card, player *target, player *saved)
            : request_base(origin_card, nullptr, target)
            , saved(saved) {}

        player *saved = nullptr;

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_add_cube : request_base, allowed_piles<card_pile_type::player_character, card_pile_type::player_table> {
        request_add_cube(card *origin_card, player *target, int ncubes = 1)
            : request_base(origin_card, nullptr, target)
            , ncubes(ncubes) {}

        int ncubes = 1;
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_move_bomb : request_base, allowed_piles<card_pile_type::player> {
        request_move_bomb(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        void on_pick(card_pile_type pile, player *target, card *target_card);
        game_formatted_string status_text() const;
    };

    struct request_rust : request_base {
        using request_base::request_base;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_card_sharper : request_base {
        request_card_sharper(card *origin_card, player *origin, player *target, card *chosen_card, card *target_card)
            : request_base(origin_card, origin, target, effect_flags::escapable)
            , chosen_card(chosen_card)
            , target_card(target_card) {}

        card *chosen_card;
        card *target_card;

        void on_resolve();
        game_formatted_string status_text() const;
    };

    struct request_ricochet : request_destroy {
        using request_destroy::request_destroy;

        std::vector<card *> barrels_used;
        game_formatted_string status_text() const;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, request_type,
        (none,          request_base)
        (predraw,       request_predraw)
        (draw,          request_draw)
        (check,         request_check)
        (generalstore,  request_generalstore)
        (discard,       request_discard)
        (discard_pass,  request_discard_pass)
        (bang,          request_bang)
        (duel,          request_duel)
        (indians,       request_indians)
        (destroy,       request_destroy)
        (steal,         request_steal)
        (death,         request_death)
        (bandidos,      request_bandidos)
        (tornado,       request_tornado)
        (poker,         request_poker)
        (poker_draw,    request_poker_draw)
        (saved,         request_saved)
        (add_cube,      request_add_cube)
        (move_bomb,     request_move_bomb)
        (rust,          request_rust)
        (card_sharper,  request_card_sharper)
        (ricochet,      request_ricochet)
        (peyote,        request_peyote)
        (handcuffs,     request_handcuffs)
        (shopchoice,    request_shopchoice)
        (kit_carlson,   request_kit_carlson)
        (claus_the_saint, request_claus_the_saint)
        (vera_custer,   request_vera_custer)
        (youl_grinner,  request_youl_grinner)
        (dutch_will,    request_dutch_will)
        (shop_choose_target, request_shop_choose_target)
        (thedaltons,    request_thedaltons)
        (lemonade_jim,  timer_lemonade_jim)
        (al_preacher,   timer_al_preacher)
        (damaging,      timer_damaging)
        (tumbleweed,    timer_tumbleweed)
    )

    template<request_type E> concept picking_request =
        requires(enums::enum_type_t<E> &req, card_pile_type pile, player *target, card *target_card) {
        { enums::enum_type_t<E>::valid_pile(pile) } -> std::convertible_to<bool>;
        req.on_pick(pile, target, target_card);
    };

    template<request_type E> concept resolvable_request = requires (enums::enum_type_t<E> &req) {
        req.on_resolve();
    };

    template<request_type E> concept timer_request = std::derived_from<enums::enum_type_t<E>, timer_base>;

    struct request_holder : enums::enum_variant<request_type> {
        using enums::enum_variant<request_type>::enum_variant;
        
        template<request_type E, typename ... Ts>
        request_holder(enums::enum_constant<E> tag, Ts && ... args)
            : enums::enum_variant<request_type>(tag, std::forward<Ts>(args) ...) {}

        card *origin_card() const {
            return enums::visit(&request_base::origin_card, *this);
        }

        player *origin() const {
            return enums::visit(&request_base::origin, *this);
        }

        player *target() const {
            return enums::visit(&request_base::target, *this);
        }

        effect_flags flags() const {
            return enums::visit(&request_base::flags, *this);
        }

        game_formatted_string status_text() const {
            return enums::visit_indexed([]<request_type T>(enums::enum_constant<T>, const auto &req) -> game_formatted_string {
                if constexpr (requires { req.status_text(); }) {
                    return req.status_text();
                } else {
                    return {};
                }
            }, *this);
        }

        bool resolvable() const {
            return enums::visit_indexed([]<request_type T>(enums::enum_constant<T>, auto) {
                return resolvable_request<T>;
            }, *this);
        }
    };

}
#endif