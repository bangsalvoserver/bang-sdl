#ifndef __HIGHNOON_SCENARIOS_H__
#define __HIGHNOON_SCENARIOS_H__

#include "../card_effect.h"

namespace banggame {

    struct effect_blessing : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_curse : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_thedaltons  {
        void on_enable(card *target_card, player *target);
    };

    struct request_thedaltons : request_base {
        request_thedaltons(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct effect_thedoctor  {
        void on_enable(card *target_card, player *target);
    };

    struct effect_trainarrival {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_thirst {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_highnoon : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_shootout : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct effect_invert_rotation  {
        void on_enable(card *target_card, player *target);
    };

    struct effect_reverend {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_hangover {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_sermon {
        void on_enable(card *target_card, player *target);
        void on_disable(card *target_card, player *target);
    };

    struct effect_ghosttown  {
        void on_enable(card *target_card, player *target);
    };

    struct effect_handcuffs : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct request_handcuffs : selection_picker {
        request_handcuffs(card *origin_card, player *target)
            : selection_picker(origin_card, nullptr, target) {}

        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };

    struct effect_newidentity : event_based_effect {
        void on_enable(card *target_card, player *target);
    };

    struct request_newidentity : request_base {
        request_newidentity(card *origin_card, player *target)
            : request_base(origin_card, nullptr, target) {}

        bool can_pick(pocket_type pocket, player *target, card *target_card) const override;
        void on_pick(pocket_type pocket, player *target, card *target_card) override;
        game_formatted_string status_text(player *owner) const override;
    };
}

#endif