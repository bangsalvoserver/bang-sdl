#include "effects_canyondiablo.h"
#include "requests_canyondiablo.h"
#include "requests_valleyofshadows.h"
#include "requests_base.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_graverobber::on_play(card *origin_card, player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            if (origin->m_game->m_discards.empty()) {
                origin->m_game->draw_card_to(card_pile_type::selection);
            } else {
                origin->m_game->move_to(origin->m_game->m_discards.back(), card_pile_type::selection);
            }
        }
        origin->m_game->queue_request<request_generalstore>(origin_card, origin, origin);
    }

    void effect_mirage::verify(card *origin_card, player *origin) const {
        if (origin->m_game->m_requests.empty()
            || origin->m_game->top_request().origin() != origin->m_game->m_playing) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_mirage::on_play(card *origin_card, player *origin) {
        origin->m_game->m_playing->skip_turn();
    }

    void effect_disarm::verify(card *origin_card, player *origin) const {
        if (origin->m_game->m_requests.empty() || !origin->m_game->top_request().origin()) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_disarm::on_play(card *origin_card, player *origin) {
        player *shooter = origin->m_game->top_request().origin();
        if (!shooter->m_hand.empty()) {
            shooter->discard_card(shooter->random_hand_card());
        }
    }

    void handler_card_sharper::verify(card *origin_card, player *origin, const mth_target_list &targets) const {
        auto chosen_card = std::get<card *>(targets[0]);
        auto [target, target_card] = targets[1];

        if (auto *c = origin->find_equipped_card(target_card)) {
            throw game_error("ERROR_DUPLICATED_CARD", c);
        }
        if (auto *c = target->find_equipped_card(chosen_card)) {
            throw game_error("ERROR_DUPLICATED_CARD", c);
        }
    }

    void handler_card_sharper::on_play(card *origin_card, player *origin, const mth_target_list &targets) {
        auto chosen_card = std::get<card *>(targets[0]);
        auto [target, target_card] = targets[1];

        if (target->can_escape(origin, origin_card, effect_flags::escapable)) {
            origin->m_game->queue_request<request_card_sharper>(origin_card, origin, target, chosen_card, target_card);
        } else {
            on_resolve(origin_card, origin, target, chosen_card, target_card);
        }
    }

    void handler_card_sharper::on_resolve(card *origin_card, player *origin, player *target, card *chosen_card, card *target_card) {
        target->unequip_if_enabled(target_card);
        origin->equip_card(target_card);
        if (chosen_card->owner == origin) {
            origin->unequip_if_enabled(chosen_card);
        }
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
            origin->m_game->pop_request<timer_damaging>();
        }
        origin->damage(origin_card, origin, 1);
        origin->m_game->queue_action([=]{
            if (origin->alive()) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                if (fatal) {
                    origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                }
            }
        });
    }

    bool effect_lastwill::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<request_death>(origin);
    }

    void effect_lastwill::on_play(card *origin_card, player *origin) {
        origin->m_game->send_request_update();
    }

    void handler_lastwill::on_play(card *origin_card, player *origin, const mth_target_list &targets) {
        player *target = std::get<player *>(targets[0]);

        for (auto [p, c] : targets | std::views::drop(1)) {
            target->add_to_hand(c);
        }
    }
}