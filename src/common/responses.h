#ifndef __RESPONSES_H__
#define __RESPONSES_H__

#include "card_effect.h"

namespace banggame {
    struct response_effect {
        virtual ~response_effect() {}

        player *origin = nullptr;
        player *target = nullptr;
    };

    using response_holder = vbase_holder<response_effect>;

    struct picking_response : response_effect {
        virtual void on_pick(card_pile_type pile, int card_id) = 0;
    };

    struct response_predraw: picking_response {
        virtual void on_pick(card_pile_type pile, int card_id) override;
    };

    struct response_draw : picking_response {
        virtual void on_pick(card_pile_type pile, int card_id) override;
    };

    struct response_check : picking_response {
        virtual void on_pick(card_pile_type pile, int card_id) override;
    };

    struct response_generalstore : picking_response {
        virtual void on_pick(card_pile_type pile, int card_id) override;
    };

    struct response_discard : picking_response {
        virtual void on_pick(card_pile_type pile, int card_id) override;
    };

    struct card_response : response_effect {
        virtual bool on_respond(card *target_card) = 0;
        virtual void on_resolve() {};
    };

    struct response_bang : card_response {
        virtual bool on_respond(card *target_card) override;
        virtual void on_resolve() override;
    };

    struct response_duel : card_response {
        virtual bool on_respond(card *target_card) override;
        virtual void on_resolve() override;
    };

    struct response_indians : card_response {
        virtual bool on_respond(card *target_card) override;
        virtual void on_resolve() override;
    };

    struct response_death : card_response {
        virtual bool on_respond(card *target_card) override;
        virtual void on_resolve() override;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, response_type,
        (none)
        (predraw,       response_predraw)
        (draw,          response_draw)
        (check,         response_check)
        (generalstore,  response_generalstore)
        (discard,       response_discard)
        (bang,          response_bang)
        (duel,          response_duel)
        (indians,       response_indians)
        (death,         response_death)
    )

}
#endif