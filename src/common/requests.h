#ifndef __REQUESTS_H__
#define __REQUESTS_H__

#include "effects/requests_base.h"
#include "effects/requests_armedanddangerous.h"
#include "effects/requests_valleyofshadows.h"
#include "effects/requests_canyondiablo.h"
#include "effects/scenarios.h"

#include "characters.h"

namespace banggame {

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
        (lastwill,      request_lastwill)
        (lastwill_target, request_lastwill_target)
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