#include "common/effects/effects_canyondiablo.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_graverobber::on_play(card *origin_card, player *origin) {
        origin->m_game->move_to(origin_card, card_pile_type::selection);
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            if (origin->m_game->m_discards.empty()) {
                origin->m_game->draw_card_to(card_pile_type::selection);
            } else {
                origin->m_game->move_to(origin->m_game->m_discards.back(), card_pile_type::selection);
            }
        }
        origin->m_game->move_to(origin_card, card_pile_type::discard_pile);
        origin->m_game->queue_request<request_type::generalstore>(origin_card, origin, origin);
    }

    void effect_mirage::verify(card *origin_card, player *origin) const {
        if (origin->m_game->m_requests.empty()
            || origin->m_game->top_request().origin() != origin->m_game->m_playing) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_mirage::on_play(card *origin_card, player *origin) {
        origin->m_game->m_playing->untap_inactive_cards();
        origin->m_game->get_next_in_turn(origin->m_game->m_playing)->start_of_turn();
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

    void effect_card_sharper_choose::verify(card *origin_card, player *origin, player *target, card *target_card) const {
        // hack che crea e ricrea l'evento solo per archiviare chosen_card
        origin->m_game->add_event<event_type::on_play_card_end>(origin_card, card_sharper_handler{origin_card, origin, nullptr, target_card, nullptr});
    }

    void effect_card_sharper_choose::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        origin->m_game->add_event<event_type::on_play_card_end>(origin_card, card_sharper_handler{origin_card, origin, nullptr, target_card, nullptr});
    }

    void effect_card_sharper_switch::verify(card *origin_card, player *origin, player *target, card *target_card) const {
        auto *chosen_card = origin->m_game->m_event_handlers.find(origin_card)->second
            .get<event_type::on_play_card_end>().target<card_sharper_handler>()->chosen_card;
        origin->m_game->remove_events(origin_card);
        
        if (auto *c = origin->find_equipped_card(target_card)) {
            throw game_error("ERROR_DUPLICATED_CARD", c);
        }
        if (auto *c = target->find_equipped_card(chosen_card)) {
            throw game_error("ERROR_DUPLICATED_CARD", c);
        }
    }

    void effect_card_sharper_switch::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        auto *handler = origin->m_game->m_event_handlers.find(origin_card)->second
            .get<event_type::on_play_card_end>().target<card_sharper_handler>();
        handler->target = target;
        handler->target_card = target_card;
    }

    void card_sharper_handler::operator()(player *p, card *c) {
        if (p == origin && c == origin_card) {
            if (target->can_escape(origin, origin_card, effect_flags::escapable)) {
                origin->m_game->queue_request<request_type::card_sharper>(origin_card, origin, target, chosen_card, target_card);
            } else {
                on_resolve();
            }
            p->m_game->remove_events(origin_card);
        }
    }

    void card_sharper_handler::on_resolve() {
        if (!origin->m_game->table_cards_disabled(target)) {
            target_card->on_unequip(target);
        }
        origin->equip_card(target_card);
        if (chosen_card->owner == origin && !origin->m_game->table_cards_disabled(origin)) {
            chosen_card->on_unequip(origin);
        }
        target->equip_card(chosen_card);
    }

    bool effect_sacrifice::can_respond(card *origin_card, player *origin) const {
        if (origin->m_game->top_request_is(request_type::damaging)) {
            auto &req = origin->m_game->top_request().get<request_type::damaging>();
            return req.origin != origin;
        }
        return false;
    }

    void effect_sacrifice::on_play(card *origin_card, player *origin) {
        auto &req = origin->m_game->top_request().get<request_type::damaging>();
        player *saved = req.origin;
        bool fatal = saved->m_hp <= req.damage;
        if (0 == --req.damage) {
            origin->m_game->pop_request(request_type::damaging);
        }
        origin->damage(origin_card, origin, 1);
        origin->m_game->queue_event<event_type::delayed_action>([=]{
            if (origin->alive()) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                if (fatal) {
                    origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                }
            }
        });
    }
}