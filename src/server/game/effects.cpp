#include "common/effects.h"
#include "common/requests.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void event_based_effect::on_unequip(player *target, int card_id) {
        target->m_game->remove_events(card_id);
    }

    void effect_bang::on_play(player *origin, player *target) {
        target->m_game->queue_request<request_type::bang>(origin, target);
    }

    static auto &make_bangcard_request(player *origin, player *target) {
        auto &req = target->m_game->queue_request<request_type::bang>(origin, target);
        req.is_bang_card = true;
        event_args args{std::in_place_index<enums::indexof(event_type::apply_bang_modifiers)>, origin, req};
        target->m_game->handle_event(args);
        return req;
    }

    void effect_bangcard::on_play(player *origin, player *target) {
        make_bangcard_request(origin, target);
    }

    void effect_aimbang::on_play(player *origin, player *target) {
        make_bangcard_request(origin, target).bang_damage = 2;
    }

    bool effect_missed::can_respond(player *origin) const {
        if (origin->m_game->top_request().is(request_type::bang)) {
            auto &req = origin->m_game->top_request().get<request_type::bang>();
            return !req.unavoidable;
        }
        return false;
    }

    void effect_missed::on_play(player *origin) {
        auto &req = origin->m_game->top_request().get<request_type::bang>();
        if (0 == --req.bang_strength) {
            origin->m_game->pop_request();
        }
    }

    bool effect_barrel::can_respond(player *origin) const {
        return effect_missed().can_respond(origin);
    }

    void effect_barrel::on_play(player *origin, player *target, int card_id) {
        auto &req = target->m_game->top_request().get<request_type::bang>();
        if (std::ranges::find(req.barrels_used, card_id) == std::ranges::end(req.barrels_used)) {
            req.barrels_used.push_back(card_id);
            target->m_game->draw_check_then(target, [target](card_suit_type suit, card_value_type) {
                if (suit == card_suit_type::hearts) {
                    effect_missed().on_play(target);
                }
            });
        }
    }

    bool effect_banglimit::can_play(player *target) const {
        return target->can_play_bang();
    }

    void effect_banglimit::on_play(player *origin) {
        ++origin->m_bangs_played;
    }

    void effect_indians::on_play(player *origin, player *target) {
        target->m_game->queue_request<request_type::indians>(origin, target);
    }

    void effect_duel::on_play(player *origin, player *target) {
        target->m_game->queue_request<request_type::duel>(origin, target);
    }

    bool effect_bangresponse::can_respond(player *origin) const {
        auto index = origin->m_game->top_request().enum_index();
        return index == request_type::duel || index == request_type::indians;
    }

    void effect_bangresponse::on_play(player *target) {
        switch (target->m_game->top_request().enum_index()) {
        case request_type::duel: {
            player *origin = target->m_game->top_request().origin();
            target->m_game->pop_request_noupdate();
            target->m_game->queue_request<request_type::duel>(target, origin);
            break;
        }
        case request_type::indians:
            target->m_game->pop_request();
        }
    }

    bool effect_bangmissed::can_respond(player *origin) const {
        return effect_missed().can_respond(origin) || effect_bangresponse().can_respond(origin);
    }

    void effect_bangmissed::on_play(player *target) {
        switch (target->m_game->top_request().enum_index()) {
        case request_type::bang:
            effect_missed().on_play(target);
            break;
        case request_type::duel:
        case request_type::indians:
            effect_bangresponse().on_play(target);
            break;
        }
    }

    void effect_generalstore::on_play(player *origin) {
        for (int i=0; i<origin->m_game->num_alive(); ++i) {
            origin->m_game->add_to_temps(origin->m_game->draw_card());
        }
        origin->m_game->queue_request<request_type::generalstore>(origin, origin);
    }

    void effect_heal::on_play(player *origin, player *target) {
        target->heal(1);
    }

    bool effect_damage::can_play(player *target) const {
        return target->m_hp > 1;
    }

    void effect_damage::on_play(player *origin, player *target) {
        target->damage(origin, 1);
    }

    void effect_beer::on_play(player *origin, player *target) {
        if (target->m_game->m_players.size() <= 2 || target->m_game->num_alive() > 2) {
            target->heal(target->m_beer_strength);
        }
    }

    bool effect_deathsave::can_respond(player *origin) const {
        return origin->m_game->top_request().is(request_type::death);
    }

    void effect_deathsave::on_play(player *origin) {
        if (origin->m_hp > 0) {
            origin->m_game->pop_request();
        }
    }

    void effect_destroy::on_play(player *origin, player *target, int card_id) {
        target->m_game->queue_event<event_type::on_discard_card>(origin, target, card_id);
        target->m_game->queue_event<event_type::do_discard_card>(origin, target, card_id);
    }

    void effect_virtual_destroy::on_play(player *origin, player *target, int card_id) {
        target->m_virtual = std::make_pair(card_id, target->discard_card(card_id));
    }

    void effect_steal::on_play(player *origin, player *target, int card_id) {
        target->m_game->queue_event<event_type::on_discard_card>(origin, target, card_id);
        target->m_game->queue_event<event_type::do_steal_card>(origin, target, card_id);
    }

    void effect_mustang::on_equip(player *target, int card_id) {
        ++target->m_distance_mod;
    }

    void effect_mustang::on_unequip(player *target, int card_id) {
        --target->m_distance_mod;
    }

    void effect_scope::on_equip(player *target, int card_id) {
        ++target->m_range_mod;
    }

    void effect_scope::on_unequip(player *target, int card_id) {
        --target->m_range_mod;
    }

    void effect_jail::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 1);
    }

    void effect_jail::on_unequip(player *target, int card_id) {
        target->remove_predraw_check(card_id);
    }

    void effect_jail::on_predraw_check(player *target, int card_id) {
        target->m_game->draw_check_then(target, [=](card_suit_type suit, card_value_type) {
            auto &moved = target->discard_card(card_id);
            if (suit == card_suit_type::hearts) {
                target->next_predraw_check(card_id);
            } else {
                target->m_game->next_turn();
            }
        });
    }

    void effect_dynamite::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 2);
    }

    void effect_dynamite::on_unequip(player *target, int card_id) {
        target->remove_predraw_check(card_id);
    }

    void effect_dynamite::on_predraw_check(player *target, int card_id) {
        target->m_game->draw_check_then(target, [=](card_suit_type suit, card_value_type value) {
            if (suit == card_suit_type::spades
                && enums::indexof(value) >= enums::indexof(card_value_type::value_2)
                && enums::indexof(value) <= enums::indexof(card_value_type::value_9)) {
                target->discard_card(card_id);
                target->damage(nullptr, 3);
            } else {
                auto moved = target->get_card_removed(card_id);
                auto *p = target;
                do {
                    p = p->m_game->get_next_player(p);
                } while (p->has_card_equipped(moved.name));
                p->equip_card(std::move(moved));
            }
            target->next_predraw_check(card_id);
        });
    }

    void effect_snake::on_equip(player *target, int card_id) {
        target->add_predraw_check(card_id, 0);
    }

    void effect_snake::on_unequip(player *target, int card_id) {
        target->remove_predraw_check(card_id);
    }

    void effect_snake::on_predraw_check(player *target, int card_id) {
        target->m_game->draw_check_then(target, [=](card_suit_type suit, card_value_type value) {
            if (suit == card_suit_type::spades) {
                target->damage(nullptr, 1);
            }
            target->next_predraw_check(card_id);
        });
    }

    void effect_weapon::on_equip(player *target, int card_id) {
        target->discard_weapon(card_id);
        target->m_weapon_range = maxdistance;
    }

    void effect_weapon::on_unequip(player *target, int card_id) {
        target->m_weapon_range = 1;
    }

    void effect_volcanic::on_equip(player *target, int card_id) {
        ++target->m_infinite_bangs;
    }

    void effect_volcanic::on_unequip(player *target, int card_id) {
        --target->m_infinite_bangs;
    }

    void effect_draw::on_play(player *origin, player *target) {
        target->add_to_hand(target->m_game->draw_card());
    }

    bool effect_draw_discard::can_play(player *target) const {
        return ! target->m_game->m_discards.empty();
    }

    void effect_draw_discard::on_play(player *origin, player *target) {
        target->add_to_hand(target->m_game->draw_from_discards());
    }

    void effect_draw_rest::on_play(player *origin, player *target) {
        for (int i=1; i<target->m_num_drawn_cards; ++i) {
            target->add_to_hand(target->m_game->draw_card());
        }
    }

    void effect_boots::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, bool is_bang){
            if (p == target) {
                target->add_to_hand(target->m_game->draw_card());
            }
        });
    }

    void effect_horsecharm::on_equip(player *target, int card_id) {
        ++target->m_num_checks;
    }

    void effect_horsecharm::on_unequip(player *target, int card_id) {
        --target->m_num_checks;
    }

    void effect_pickaxe::on_equip(player *target, int card_id) {
        ++target->m_num_drawn_cards;
    }

    void effect_pickaxe::on_unequip(player *target, int card_id) {
        --target->m_num_drawn_cards;
    }

    void effect_calumet::on_equip(player *target, int card_id) {
        ++target->m_calumets;
    }

    void effect_calumet::on_unequip(player *target, int card_id) {
        --target->m_calumets;
    }

    void effect_shotgun::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, bool is_bang) {
            if (origin == p && target != p && !target->m_hand.empty() && is_bang) {
                target->m_game->queue_request<request_type::discard>(origin, target);
            }
        });
    }

    void effect_bounty::on_equip(player *p, int card_id) {
        p->m_game->add_event<event_type::on_hit>(card_id, [p](player *origin, player *target, bool is_bang) {
            if (origin && target == p && is_bang) {
                origin->add_to_hand(origin->m_game->draw_card());
            }
        });
    }

    void effect_bandidos::on_play(player *origin, player *target) {
        target->m_game->queue_request<request_type::bandidos>(origin, target);
    }

    void effect_tornado::on_play(player *origin, player *target) {
        if (target->num_hand_cards() == 0) {
            target->add_to_hand(target->m_game->draw_card());
            target->add_to_hand(target->m_game->draw_card());
        } else {
            target->m_game->queue_request<request_type::tornado>(origin, target);
        }
    }

    void effect_poker::on_play(player *origin) {
        auto next = origin;
        do {
            next = origin->m_game->get_next_player(next);
            if (next == origin) return;
        } while (next->m_hand.empty());
        origin->m_game->queue_request<request_type::poker>(origin, next);
    }
}