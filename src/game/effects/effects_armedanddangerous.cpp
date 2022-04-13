#include "effects_armedanddangerous.h"
#include "equips_armedanddangerous.h"
#include "requests_armedanddangerous.h"
#include "effects_base.h"
#include "requests_base.h"

#include "../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void handler_draw_atend::on_play(card *origin_card, player *origin, const mth_target_list &targets) {
        for (auto [target, _] : targets) {
            target->m_game->draw_card_to(pocket_type::player_hand, target);
        }
    }

    void effect_select_cube::verify(card *origin_card, player *origin, player *target, card *target_card) const {
        if (target_card->cubes.size() < 1) {
            throw game_error("ERROR_NOT_ENOUGH_CUBES_ON", target_card);
        }
    }

    void effect_select_cube::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->pay_cubes(target_card, 1);
    }

    bool effect_pay_cube::can_respond(card *origin_card, player *origin) const {
        return origin_card->cubes.size() >= ncubes;
    }

    void effect_pay_cube::verify(card *origin_card, player *origin) const {
        if (origin_card->cubes.size() < ncubes) {
            throw game_error("ERROR_NOT_ENOUGH_CUBES_ON", origin_card);
        }
    }

    void effect_pay_cube::on_play(card *origin_card, player *origin) {
        origin->pay_cubes(origin_card, ncubes);
    }

    void effect_add_cube::on_play(card *origin_card, player *origin, player *target, card *target_card) {
        target->add_cubes(target_card, ncubes);
    }

    void effect_reload::on_play(card *origin_card, player *origin) {
        if (origin->can_receive_cubes()) {
            origin->m_game->queue_request<request_add_cube>(origin_card, origin, 3);
        }
    }
    
    void effect_rust::on_play(card *origin_card, player *origin, player *target, effect_flags flags) {
        if (target->count_cubes() == 0) return;
        if (target->can_escape(origin, origin_card, flags)) {
            origin->m_game->queue_request<request_rust>(origin_card, origin, target, flags);
        } else {
            on_resolve(origin_card, origin, target);
        }
    }

    void effect_rust::on_resolve(card *origin_card, player *origin, player *target) {
        auto view = target->m_table | std::views::filter([](card *c){ return c->color == card_color_type::orange; });
        std::vector<card *> orange_cards{view.begin(), view.end()};
        
        orange_cards.push_back(target->m_characters.front());
        for (card *c : orange_cards) {
            target->move_cubes(c, origin->m_characters.front(), 1);
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

        auto clear_flags = [=](player *p) {
            if (p == origin) {
                origin->remove_player_flags(player_flags::see_everyone_range_1);
                origin->m_game->remove_events(origin_card);
            }
        };

        origin->m_game->add_event<event_type::on_turn_end>(origin_card, clear_flags);
        origin->m_game->add_event<event_type::on_effect_end>(origin_card, [=](player *p, card *target_card) {
            if (origin_card != target_card) {
                clear_flags(p);
            }
        });
    }

    void effect_doublebarrel::on_play(card *origin_card, player *origin) {
        origin->m_game->add_event<event_type::apply_bang_modifier>(origin_card, [=](player *p, request_bang *req) {
            if (p == origin) {
                req->unavoidable = origin->get_card_sign(req->origin_card).suit == card_suit::diamonds;
                origin->m_game->remove_events(origin_card);
            }
        });
    }

    void effect_thunderer::on_play(card *origin_card, player *origin) {
        origin->m_game->add_event<event_type::apply_bang_modifier>(origin_card, [=](player *p, request_bang *req) {
            if (p == origin) {
                card *bang_card = req->origin->chosen_card_or(req->origin_card);
                req->origin->m_game->move_to(bang_card, pocket_type::player_hand, true, req->origin, show_card_flags::short_pause | show_card_flags::show_everyone);

                req->on_cleanup([origin = req->origin, bang_card]{
                    if (bang_card->owner == origin) {
                        origin->m_game->send_card_update(*bang_card, origin);
                    }
                });

                origin->m_game->remove_events(origin_card);
            }
        });
    }

    void effect_buntlinespecial::on_play(card *origin_card, player *p) {
        p->m_game->add_event<event_type::apply_bang_modifier>(origin_card, [=](player *origin, request_bang *req) {
            if (p == origin) {
                p->m_game->add_event<event_type::on_missed>(origin_card, [=](card *bang_card, player *origin, player *target, bool is_bang) {
                    if (target && origin == p && is_bang && !target->m_hand.empty()) {
                        target->m_game->queue_request<request_discard>(origin_card, origin, target);
                    }
                });
                req->on_cleanup([=]{
                    p->m_game->remove_events(origin_card);
                });
            }
        });
    }

    void effect_bigfifty::on_play(card *origin_card, player *p) {
        p->m_game->add_disabler(origin_card, [=](card *c) {
            return (c->pocket == pocket_type::player_table
                || c->pocket == pocket_type::player_character)
                && c->owner != p;
        });
        p->m_game->add_event<event_type::apply_bang_modifier>(origin_card, [=](player *origin, request_bang *req) {
            if (origin == p) {
                req->on_cleanup([=]{
                    p->m_game->remove_disablers(origin_card);
                });
                origin->m_game->remove_events(origin_card);
            }
        });
    }

    void effect_flintlock::on_play(card *origin_card, player *p) {
        p->m_game->add_event<event_type::on_missed>(origin_card, [=](card *origin_card, player *origin, player *target, bool is_bang) {
            if (origin == p) {
                origin->add_to_hand(origin_card);
            }
        });
        p->m_game->top_request().get<request_bang>().on_cleanup([=]{
            p->m_game->remove_events(origin_card);
        });
    }

    void effect_duck::on_play(card *origin_card, player *origin) {
        origin->add_to_hand(origin_card);
    }

    void handler_squaw::verify(card *origin_card, player *origin, const mth_target_list &targets) const {
        if (targets.size() == 3) {
            auto discarded_card = std::get<card *>(targets[0]);
            for (auto [target, target_card] : targets | std::views::drop(1)) {
                if (target_card == discarded_card) {
                    throw game_error("ERROR_INVALID_ACTION");
                }
                effect_select_cube().verify(origin_card, origin, target, target_card);
            };
        }
    }

    void handler_squaw::on_play(card *origin_card, player *origin, const mth_target_list &targets) {
        auto [target, target_card] = targets[0];

        bool immune = target->immune_to(origin_card);
        if (targets.size() == 3) {
            effect_select_cube().on_play(origin_card, origin, std::get<player *>(targets[1]), std::get<card *>(targets[1]));
            effect_select_cube().on_play(origin_card, origin, std::get<player *>(targets[2]), std::get<card *>(targets[2]));

            if (!immune) {
                effect_steal{}.on_play(origin_card, origin, target, target_card, effect_flags::escapable | effect_flags::single_target);
            }
        } else if (!immune) {
            effect_destroy{}.on_play(origin_card, origin, target, target_card, effect_flags::escapable | effect_flags::single_target);
        }
    }

    void effect_tumbleweed::on_equip(card *target_card, player *origin) {
        origin->m_game->add_event<event_type::on_draw_check_select>(target_card, [=](card *origin_card, card *drawn_card) {
            origin->m_game->queue_request_front<timer_tumbleweed>(target_card, origin, drawn_card, origin_card);
        });
    }

    bool effect_tumbleweed::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<timer_tumbleweed>(origin);
    }

    void effect_tumbleweed::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request_noupdate<timer_tumbleweed>();
        origin->m_game->do_draw_check();
        origin->m_game->update_request();
    }

    void timer_tumbleweed::on_finished() {
        target->m_game->pop_request_noupdate<timer_tumbleweed>();
        target->m_game->m_current_check->function(drawn_card);
        target->m_game->m_current_check.reset();
        target->m_game->update_request();
    }

    bool effect_move_bomb::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<request_move_bomb>(origin);
    }

    void handler_move_bomb::on_play(card *origin_card, player *origin, const mth_target_list &targets) {
        auto target = std::get<player *>(targets[0]);
        if (!target->immune_to(origin_card)) {
            if (target == origin) {
                origin->m_game->pop_request<request_move_bomb>();
            } else if (!target->find_equipped_card(origin_card)) {
                origin->unequip_if_enabled(origin_card);
                target->equip_card(origin_card);
                origin->m_game->pop_request<request_move_bomb>();
            }
        } else {
            origin->discard_card(origin_card);
            origin->m_game->pop_request<request_move_bomb>();
        }
    }
}