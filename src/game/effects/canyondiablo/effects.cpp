#include "effects.h"

#include "../valleyofshadows/requests.h"
#include "../base/requests.h"

#include "../../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_graverobber::on_play(card *origin_card, player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            if (origin->m_game->m_discards.empty()) {
                origin->m_game->draw_card_to(pocket_type::selection);
            } else {
                origin->m_game->move_card(origin->m_game->m_discards.back(), pocket_type::selection);
            }
        }
        origin->m_game->queue_request<request_generalstore>(origin_card, origin, origin);
    }

    opt_error effect_mirage::verify(card *origin_card, player *origin) const {
        if (origin->m_game->m_requests.empty()
            || origin->m_game->top_request().origin() != origin->m_game->m_playing) {
            return game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
        return std::nullopt;
    }

    void effect_mirage::on_play(card *origin_card, player *origin) {
        origin->m_game->add_log("LOG_SKIP_TURN", origin->m_game->m_playing);
        origin->m_game->m_playing->skip_turn();
    }

    opt_error effect_disarm::verify(card *origin_card, player *origin) const {
        if (origin->m_game->m_requests.empty() || !origin->m_game->top_request().origin()) {
            return game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
        return std::nullopt;
    }

    void effect_disarm::on_play(card *origin_card, player *origin) {
        player *shooter = origin->m_game->top_request().origin();
        if (!shooter->m_hand.empty()) {
            card *hand_card = shooter->random_hand_card();
            origin->m_game->add_log("LOG_DISCARDED_CARD_FOR", origin_card, shooter, hand_card);
            shooter->discard_card(hand_card);
        }
    }

    opt_error handler_card_sharper::verify(card *origin_card, player *origin, const target_list &targets) const {
        auto chosen_card = std::get<target_card_t>(targets[0]).target;
        auto target_card = std::get<target_card_t>(targets[1]).target;

        if (auto *c = origin->find_equipped_card(target_card)) {
            return game_error("ERROR_DUPLICATED_CARD", c);
        }
        if (auto *c = target_card->owner->find_equipped_card(chosen_card)) {
            return game_error("ERROR_DUPLICATED_CARD", c);
        }
        return std::nullopt;
    }

    struct request_card_sharper : request_targeting {
        request_card_sharper(card *origin_card, player *origin, player *target, card *chosen_card, card *target_card)
            : request_targeting(origin_card, origin, target, target_card, effect_flags::escapable)
            , chosen_card(chosen_card) {}

        card *chosen_card;

        void on_resolve() override {
            target->m_game->pop_request<request_card_sharper>();
            handler_card_sharper{}.on_resolve(origin_card, origin, chosen_card, target_card);
        }

        game_formatted_string status_text(player *owner) const override {
            if (target == owner) {
                return {"STATUS_CARD_SHARPER", origin_card, target_card, chosen_card};
            } else {
                return {"STATUS_CARD_SHARPER_OTHER", target, origin_card, target_card, chosen_card};
            }
        }
    };

    void handler_card_sharper::on_play(card *origin_card, player *origin, const target_list &targets) {
        auto chosen_card = std::get<target_card_t>(targets[0]).target;
        auto target_card = std::get<target_card_t>(targets[1]).target;

        if (target_card->owner->can_escape(origin, origin_card, effect_flags::escapable)) {
            origin->m_game->queue_request<request_card_sharper>(origin_card, origin, target_card->owner, chosen_card, target_card);
        } else {
            on_resolve(origin_card, origin, chosen_card, target_card);
        }
    }

    void handler_card_sharper::on_resolve(card *origin_card, player *origin, card *chosen_card, card *target_card) {
        player *target = target_card->owner;
        origin->m_game->add_log("LOG_SWAP_CARDS", origin, target, chosen_card, target_card);

        target->disable_equip(target_card);
        target_card->on_equip(origin);
        origin->equip_card(target_card);
        if (chosen_card->owner == origin) {
            origin->disable_equip(chosen_card);
        }
        chosen_card->on_equip(target);
        target->equip_card(chosen_card);
    }

    bool effect_sacrifice::can_respond(card *origin_card, player *origin) const {
        if (auto *req = origin->m_game->top_request_if<timer_damaging>()) {
            return req->target != origin;
        }
        return false;
    }

    void effect_sacrifice::on_play(card *origin_card, player *origin) {
        auto &req = origin->m_game->top_request().get<timer_damaging>();
        player *saved = req.target;
        bool fatal = saved->m_hp <= req.damage;
        if (0 == --req.damage) {
            origin->m_game->pop_request_noupdate<timer_damaging>();
        }
        origin->damage(origin_card, origin, 1);
        origin->m_game->queue_action_front([=]{
            if (origin->alive()) {
                origin->draw_card(2 + fatal, origin_card);
            }
        });
    }

    bool effect_lastwill::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<request_death>(origin);
    }

    void effect_lastwill::on_play(card *origin_card, player *origin) {
        origin->m_game->update_request();
    }

    void handler_lastwill::on_play(card *origin_card, player *origin, const target_list &targets) {
        player *target = std::get<target_player_t>(targets[0]).target;

        for (auto c : targets | std::views::drop(1)) {
            card *chosen_card = std::get<target_card_t>(c).target;
            if (chosen_card->pocket == pocket_type::player_hand) {
                origin->m_game->add_log(update_target::includes(origin, target), "LOG_GIFTED_CARD", origin, target, chosen_card);
                origin->m_game->add_log(update_target::excludes(origin, target), "LOG_GIFTED_A_CARD", origin, target);
            } else {
                origin->m_game->add_log("LOG_GIFTED_CARD", origin, target, chosen_card);
            }
            target->add_to_hand(chosen_card);
        }
    }
}