#include "common/effects/effects_armedanddangerous.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    struct draw_atend_handler {
        player *origin;
        int ncards = 1;

        void operator()(player *target, card *origin_card) {
            if (origin == target) {
                for (int i=0; i<ncards; ++i) {
                    target->m_game->draw_card_to(card_pile_type::player_hand, target);
                }
                target->m_game->remove_events(origin_card);
            }
        }
    };

    void effect_draw_atend::on_play(card *origin_card, player *origin, player *target) {
        auto it = origin->m_game->m_event_handlers.find(origin_card);
        if (it == origin->m_game->m_event_handlers.end()) {
            origin->m_game->add_event<event_type::on_play_card_end>(origin_card, draw_atend_handler{target});
        } else {
            ++it->second.get<event_type::on_play_card_end>().target<draw_atend_handler>()->ncards;
        }
    }

    void effect_pay_cube::verify(card *origin_card, player *origin, player *target, card *target_card) const {
        if (target_card->cubes.size() < std::max<short>(1, args)) {
            throw game_error("ERROR_NOT_ENOUGH_CUBES_ON", origin_card);
        }
    }

    void effect_pay_cube::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->pay_cubes(target_card, std::max<short>(1, args));
    }

    void effect_add_cube::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->add_cubes(target_card, std::max<short>(1, args));
    }

    void effect_reload::on_play(card *origin_card, player *origin) {
        if (origin->can_receive_cubes()) {
            origin->m_game->queue_request<request_type::add_cube>(origin_card, origin, 3);
        }
    }
    
    void effect_rust::on_play(card *origin_card, player *origin, player *target) {
        if (target->count_cubes() == 0) return;
        if (target->can_escape(origin, origin_card, flags)) {
            origin->m_game->queue_request<request_type::rust>(origin_card, origin, target, flags);
        } else {
            auto view = target->m_table | std::views::filter([](card *c){ return c->color == card_color_type::orange; });
            std::vector<card *> orange_cards{view.begin(), view.end()};
            
            orange_cards.push_back(target->m_characters.front());
            for (card *c : orange_cards) {
                target->move_cubes(c, origin->m_characters.front(), 1);
            }
        }
    }

    void effect_bandolier::verify(card *origin_card, player *origin) const {
        if (origin->m_bangs_played == 0) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_bandolier::on_play(card *origin_card, player *origin) {
        ++origin->m_bangs_per_turn;
    }

    void effect_belltower::verify(card *origin_card, player *origin) const {
        if (origin->check_player_flags(player_flags::see_everyone_range_1)) {
            throw game_error("ERROR_CANT_PLAY_CARD", origin_card);
        }
    }

    void effect_belltower::on_play(card *origin_card, player *origin) {
        origin->add_player_flags(player_flags::see_everyone_range_1);

        origin->m_game->add_event<event_type::on_play_card_end>(origin_card, [=](player *p, card *target_card) {
            if (p == origin && origin_card != target_card) {
                origin->remove_player_flags(player_flags::see_everyone_range_1);
                origin->m_game->remove_events(origin_card);
            }
        });

        origin->m_game->add_event<event_type::on_turn_end>(origin_card, [=](player *p) {
            if (p == origin) {
                origin->remove_player_flags(player_flags::see_everyone_range_1);
                origin->m_game->remove_events(origin_card);
            }
        });
    }

    void effect_doublebarrel::on_play(card *origin_card, player *origin) {
        origin->add_bang_mod([=](request_bang &req) {
            if (origin->get_card_suit(req.origin_card) == card_suit_type::diamonds) {
                req.unavoidable = true;
            }
        });
    }

    void effect_thunderer::on_play(card *origin_card, player *origin) {
        origin->add_bang_mod([](request_bang &req) {
            card *bang_card = req.origin->m_chosen_card ? req.origin->m_chosen_card : req.origin_card;
            req.origin->m_game->move_to(bang_card, card_pile_type::player_hand, true, req.origin, show_card_flags::short_pause | show_card_flags::show_everyone);

            req.cleanup_function = [origin = req.origin, bang_card]{
                origin->m_game->send_card_update(*bang_card, origin);
            };
        });
    }

    void effect_buntlinespecial::on_play(card *origin_card, player *p) {
        p->add_bang_mod([=](request_bang &req) {
            p->m_game->add_event<event_type::on_missed>(origin_card, [=](card *bang_card, player *origin, player *target, bool is_bang) {
                if (target && origin == p && is_bang && !target->m_hand.empty()) {
                    target->m_game->queue_request<request_type::discard>(origin_card, origin, target);
                }
            });
            req.cleanup_function = [=]{
                p->m_game->remove_events(origin_card);
            };
        });
    }

    void effect_bigfifty::on_play(card *origin_card, player *p) {
        p->m_game->add_disabler(origin_card, [=](card *c) {
            return (c->pile == card_pile_type::player_table
                || c->pile == card_pile_type::player_character)
                && c->owner != p;
        });
        p->add_bang_mod([=](request_bang &req) {
            req.cleanup_function = [=]{
                p->m_game->remove_disablers(origin_card);
            };
        });
    }

    void effect_flintlock::on_play(card *origin_card, player *p) {
        p->m_game->add_event<event_type::on_missed>(origin_card, [=](card *origin_card, player *origin, player *target, bool is_bang) {
            if (origin == p) {
                origin->add_to_hand(origin_card);
            }
        });
        p->m_game->top_request().get<request_type::bang>().cleanup_function = [=]{
            p->m_game->remove_events(origin_card);
        };
    }

    void effect_duck::on_play(card *origin_card, player *origin) {
        origin->add_to_hand(origin_card);
    }

    struct squaw_handler {
        card *origin_card;
        player *origin;
        player *target;
        card *target_card;
        effect_flags flags;
        bool steal;

        void operator()(player *p, card *c) {
            if (p == origin && c == origin_card) {
                if (steal) {
                    effect_steal e;
                    e.flags = flags;
                    e.on_play(origin_card, origin, target, target_card);
                } else {
                    effect_destroy e;
                    e.flags = flags;
                    e.on_play(origin_card, origin, target, target_card);
                }
                origin->m_game->remove_events(origin_card);
            }
        }
    };

    void effect_squaw_destroy::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        origin->m_game->add_event<event_type::on_play_card_end>(origin_card,
            squaw_handler{origin_card, origin, target, target_card, flags, false});
    }

    void effect_squaw_steal::on_play(card *origin_card, player *origin) {
        origin->m_game->m_event_handlers.find(origin_card)->second
            .get<event_type::on_play_card_end>().target<squaw_handler>()->steal = true;
    }

    void effect_tumbleweed::on_equip(card *target_card, player *origin) {
        origin->m_game->add_event<event_type::trigger_tumbleweed>(target_card, [=](card *origin_card, card *drawn_card) {
            origin->m_game->add_request<request_type::tumbleweed>(target_card, origin, drawn_card, origin_card);
        });
    }

    bool effect_tumbleweed::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is(request_type::tumbleweed);
    }

    void effect_tumbleweed::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request_noupdate(request_type::tumbleweed);
        origin->m_game->do_draw_check();
        origin->m_game->events_after_requests();
    }

    void timer_tumbleweed::on_finished() {
        target->m_game->pop_request_noupdate(request_type::tumbleweed);
        target->m_game->m_current_check->function(drawn_card);
        target->m_game->m_current_check.reset();
        target->m_game->events_after_requests();
    }
}