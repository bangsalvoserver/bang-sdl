#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include "effect_holder.h"
#include "characters.h"
#include "format_str.h"
#include "timer.h"

namespace banggame {

    struct request_predraw : picking_request_allowing<card_pile_type::player_table> {
        request_predraw(player *target) {
            request_base::target = target;
        }
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_check : picking_request_allowing<card_pile_type::selection> {
        request_check(card *origin_card, player *target) {
            request_base::origin_card = origin_card;
            request_base::target = target;
        }

        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_generalstore : picking_request_allowing<card_pile_type::selection> {
        request_generalstore(card *origin_card, player *origin, player *target) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
        }

        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_discard : picking_request_allowing<card_pile_type::player_hand> {
        request_discard(card *origin_card, player *origin, player *target) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
        }
        int ncards = 1;
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_discard_pass : picking_request_allowing<card_pile_type::player_hand> {
        request_discard_pass(player *target) {
            request_base::target = target;
        }

        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_damaging : request_base {
        request_damaging(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
            request_base::flags = flags;
        }

        void on_resolve();
    };

    struct request_duel : request_damaging {
        request_duel(card *origin_card, player *origin, player *target, player *respond_to, effect_flags flags = no_effect_flags)
            : request_damaging(origin_card, origin, target, flags)
            , respond_to(respond_to) {}

        player *respond_to = nullptr;
    };

    struct request_bang : request_base {
        request_bang(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
            request_base::flags = flags;
        }

        std::vector<card *> barrels_used;
        int bang_strength = 1;
        int bang_damage = 1;
        bool unavoidable = false;
        bool is_bang_card = false;

        std::function<void()> cleanup_function;

        void on_resolve();
        void cleanup();
    };

    struct request_destroy : request_base {
        request_destroy(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags = no_effect_flags) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
            request_base::flags = flags;

            request_destroy::target_card = target_card;
        }
        card *target_card;

        void on_resolve();
    };

    struct request_steal : request_base {
        request_steal(card *origin_card, player *origin, player *target, card *target_card, effect_flags flags = no_effect_flags) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
            request_base::flags = flags;

            request_steal::target_card = target_card;
        }
        
        card *target_card;

        void on_resolve();
    };

    struct request_death : request_base {
        request_death(card *origin_card, player *origin, player *target) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
        }

        std::vector<card *> draw_attempts;
        
        void on_resolve();
    };

    struct request_bandidos : picking_request_allowing<card_pile_type::player_hand> {
        request_bandidos(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
            request_base::flags = flags;
        }

        int num_cards = 2;

        void on_pick(card_pile_type pile, player *target, card *target_card);
        void on_resolve();
    };

    struct request_tornado : picking_request_allowing<card_pile_type::player_hand> {
        request_tornado(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
            request_base::flags = flags;
        }
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_poker : picking_request_allowing<card_pile_type::player_hand> {
        request_poker(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
            request_base::flags = flags;
        }

        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_poker_draw : picking_request_allowing<card_pile_type::selection> {
        request_poker_draw(card *origin_card, player *origin) {
            request_base::origin_card = origin_card;
            request_base::target = origin;
        }

        int num_cards = 2;

        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_saved : picking_request_allowing<card_pile_type::player_hand, card_pile_type::main_deck> {
        request_saved(card *origin_card, player *target, player *saved) {
            request_base::origin_card = origin_card;
            request_base::target = target;
            request_saved::saved = saved;
        }

        player *saved = nullptr;

        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_add_cube : picking_request_allowing<card_pile_type::player_character, card_pile_type::player_table> {
        request_add_cube(card *origin_card, player *target, int ncubes = 1) {
            request_base::origin_card = origin_card;
            request_base::target = target;
            request_add_cube::ncubes = ncubes;
        }

        int ncubes = 1;
        
        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_move_bomb : picking_request_allowing<card_pile_type::player> {
        request_move_bomb(card *origin_card, player *target) {
            request_base::origin_card = origin_card;
            request_base::target = target;
        }

        void on_pick(card_pile_type pile, player *target, card *target_card);
    };

    struct request_rust : request_base {
        request_rust(card *origin_card, player *origin, player *target, effect_flags flags = no_effect_flags) {
            request_base::origin_card = origin_card;
            request_base::origin = origin;
            request_base::target = target;
            request_base::flags = flags;
        }

        void on_resolve();
    };

    struct request_ricochet : request_destroy {
        using request_destroy::request_destroy;

        std::vector<card *> barrels_used;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, request_type,
        (none,          request_base)
        (predraw,       request_predraw)
        (check,         request_check)
        (generalstore,  request_generalstore)
        (discard,       request_discard)
        (discard_pass,  request_discard_pass)
        (bang,          request_bang)
        (duel,          request_duel)
        (indians,       request_damaging)
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
        (ricochet,      request_ricochet)
        (peyote,        request_peyote)
        (handcuffs,     request_handcuffs)
        (shopchoice,    request_base)
        (kit_carlson,   request_kit_carlson)
        (claus_the_saint, request_claus_the_saint)
        (vera_custer,   request_vera_custer)
        (youl_grinner,  request_youl_grinner)
        (dutch_will,    request_dutch_will)
        (shop_choose_target, request_shop_choose_target)
        (thedaltons,    request_thedaltons)
        (lemonade_jim,  timer_base)
        (al_preacher,   timer_base)
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

        game_formatted_string status_text() const;

        effect_flags flags() const {
            return enums::visit(&request_base::flags, *this);
        }
    };

}
#endif