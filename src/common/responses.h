#ifndef __RESPONSES_H__
#define __RESPONSES_H__

#include "card_effect.h"
#include "characters.h"
#include "game_action.h"

namespace banggame {

    struct response_predraw: picking_response {
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

    struct response_duel : picking_response {
        virtual void on_pick(card_pile_type pile, int card_id) override;
        virtual void on_resolve() override;
    };

    struct response_indians : picking_response {
        virtual void on_pick(card_pile_type pile, int card_id) override;
        virtual void on_resolve() override;
    };

    struct response_bang_data {
        int barrels_used[5] = {};
        int bang_strength = 1;
    };

    struct response_bang : card_response {
        response_bang() {
            static_assert(sizeof(data) >= sizeof(response_bang_data));
            new (data) response_bang_data;
        }
        response_bang_data *get_data() {
            return reinterpret_cast<response_bang_data *>(data);
        }

        virtual void on_respond(const play_card_args &args) override;
        virtual void on_resolve() override;
        void handle_missed();
    };

    struct response_death : card_response {
        virtual void on_respond(const play_card_args &args) override;
        virtual void on_resolve() override;
    };

    DEFINE_ENUM_TYPES_IN_NS(banggame, response_type,
        (none)
        (predraw,       response_predraw)
        (check,         response_check)
        (generalstore,  response_generalstore)
        (discard,       response_discard)
        (bang,          response_bang)
        (duel,          response_duel)
        (indians,       response_indians)
        (death,         response_death)
        (kit_carlson,   response_kit_carlson)
    )

}
#endif