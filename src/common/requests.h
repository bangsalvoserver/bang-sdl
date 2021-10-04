#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include "effect_holder.h"
#include "characters.h"
#include "game_action.h"

namespace banggame {

    struct request_predraw : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };

    struct request_check : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };

    struct request_generalstore : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };

    struct request_discard : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };

    struct request_discard_pass : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };

    struct request_damaging : request_base {
        void on_resolve();
    };

    struct request_bang : request_base {
        std::vector<int> barrels_used;
        int bang_strength = 1;
        int bang_damage = 1;
        bool unavoidable = false;
        bool is_bang_card = false;

        void on_resolve();
    };

    struct request_death : request_base {
        void on_resolve();
    };

    struct request_bandidos : request_damaging {
        int num_cards = 2;
        void on_pick(card_pile_type pile, int card_id);
    };

    struct request_tornado : request_base {
        void on_pick(card_pile_type pile, int card_id);
    };

    struct request_poker : request_base {
        int num_cards = 2;
        void on_pick(card_pile_type pile, int card_id);
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, request_type,
        (none,          request_base)
        (predraw,       request_predraw)
        (check,         request_check)
        (generalstore,  request_generalstore)
        (discard,       request_discard)
        (discard_pass,  request_discard_pass)
        (bang,          request_bang)
        (duel,          request_damaging)
        (indians,       request_damaging)
        (death,         request_death)
        (bandidos,      request_bandidos)
        (tornado,       request_tornado)
        (poker,         request_poker)
        (kit_carlson,   request_kit_carlson)
        (claus_the_saint, request_claus_the_saint)
        (vera_custer,   request_vera_custer)
    )

    template<request_type E> concept picking_request = requires (enums::enum_type_t<E> &req, card_pile_type pile, int card_id) {
        req.on_pick(pile, card_id);
    };

    template<request_type E> concept resolvable_request = requires (enums::enum_type_t<E> &req) {
        req.on_resolve();
    };

    struct request_holder : enums::enum_variant<request_type> {
        template<request_type E> request_holder(enums::enum_constant<E> tag, player *origin, player *target)
            : enums::enum_variant<request_type>(tag) {
            auto &obj = get<E>();
            obj.origin = origin;
            obj.target = target;
        }

        player *origin() const {
            return enums::visit([]<request_type E>(enums::enum_constant<E>, const auto &obj) {
                return obj.origin;
            }, *this);
        }

        player *target() const {
            return enums::visit([]<request_type E>(enums::enum_constant<E>, const auto &obj) {
                return obj.target;
            }, *this);
        }
    };

}
#endif