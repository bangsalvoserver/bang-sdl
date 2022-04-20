#include "effects.h"
#include "equips.h"
#include "requests.h"

#include "../base/effects.h"
#include "../base/requests.h"

#include "../../game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void handler_draw_atend::on_play(card *origin_card, player *origin, const target_list &targets) {
        effect_draw(targets.size()).on_play(origin_card, origin);
    }

    opt_fmt_str handler_heal_multi::on_prompt(card *origin_card, player *origin, const target_list &targets) const {
        return effect_heal(targets.size()).on_prompt(origin_card, origin);
    }

    void handler_heal_multi::on_play(card *origin_card, player *origin, const target_list &targets) {
        effect_heal(targets.size()).on_play(origin_card, origin);
    }

    void effect_select_cube::verify(card *origin_card, player *origin, card *target) const {
        if (target->cubes.size() < 1) {
            throw game_error("ERROR_NOT_ENOUGH_CUBES_ON", target);
        }
    }

    void effect_select_cube::on_play(card *origin_card, player *origin, card *target) {
        target->owner->pay_cubes(target, 1);
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

    void effect_add_cube::on_play(card *origin_card, player *origin, card *target) {
        target->owner->add_cubes(target, ncubes);
    }

    void effect_reload::on_play(card *origin_card, player *origin) {
        origin->queue_request_add_cube(origin_card, 3);
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
                req->origin->m_game->move_card(bang_card, pocket_type::player_hand, req->origin, show_card_flags::short_pause | show_card_flags::shown);

                req->on_cleanup([origin = req->origin, bang_card]{
                    if (bang_card->owner == origin) {
                        origin->m_game->send_card_update(bang_card, origin);
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
        origin->m_game->update_request();
    }

    void handler_squaw::verify(card *origin_card, player *origin, const target_list &targets) const {
        if (targets.size() == 3) {
            auto discarded_card = std::get<target_card_t>(targets[0]).target;
            for (auto target : targets | std::views::drop(1)) {
                card *target_card = std::get<target_card_t>(target).target;
                if (target_card == discarded_card) {
                    throw game_error("ERROR_INVALID_ACTION");
                }
                effect_select_cube().verify(origin_card, origin, target_card);
            };
        }
    }

    void handler_squaw::on_play(card *origin_card, player *origin, const target_list &targets) {
        card *target_card = std::get<target_card_t>(targets[0]).target;

        bool immune = target_card->owner->immune_to(origin_card);
        if (targets.size() == 3) {
            effect_select_cube().on_play(origin_card, origin, std::get<target_card_t>(targets[1]).target);
            effect_select_cube().on_play(origin_card, origin, std::get<target_card_t>(targets[2]).target);

            if (!immune) {
                effect_steal{}.on_play(origin_card, origin, target_card, effect_flags::escapable | effect_flags::single_target);
            }
        } else if (!immune) {
            effect_destroy{}.on_play(origin_card, origin, target_card, effect_flags::escapable | effect_flags::single_target);
        }
    }

    void effect_tumbleweed::on_equip(card *target_card, player *target) {
        target->m_game->add_event<event_type::on_draw_check_select>(target_card, [=](player *origin, card *origin_card, card *drawn_card) {
            target->m_game->queue_request_front<timer_tumbleweed>(target_card, origin, target, drawn_card, origin_card);
        });
    }

    bool effect_tumbleweed::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<timer_tumbleweed>(origin);
    }

    void effect_tumbleweed::on_play(card *origin_card, player *origin) {
        origin->m_game->pop_request_noupdate<timer_tumbleweed>();
        origin->m_game->m_current_check.restart();
        origin->m_game->update_request();
    }

    void timer_tumbleweed::on_finished() {
        target->m_game->pop_request_noupdate<timer_tumbleweed>();
        origin->m_game->m_current_check.resolve(drawn_card);
        target->m_game->update_request();
    }

    bool effect_move_bomb::can_respond(card *origin_card, player *origin) const {
        return origin->m_game->top_request_is<request_move_bomb>(origin);
    }

    opt_fmt_str handler_move_bomb::on_prompt(card *origin_card, player *origin, const target_list &targets) const {
        auto target = std::get<target_player_t>(targets[0]).target;
        if (origin == target) {
            return game_formatted_string{"PROMPT_MOVE_BOMB_TO_SELF", origin_card};
        } else {
            return std::nullopt;
        }
    }

    void handler_move_bomb::verify(card *origin_card, player *origin, const target_list &targets) const {
        auto target = std::get<target_player_t>(targets[0]).target;
        if (target != origin) {
            if (auto c = target->find_equipped_card(origin_card)) {
                throw game_error("ERROR_DUPLICATED_CARD", c);
            }
        }
    }

    void handler_move_bomb::on_play(card *origin_card, player *origin, const target_list &targets) {
        player *target = std::get<target_player_t>(targets[0]).target;
        if (target->immune_to(origin_card)) {
            // Non ne sono tanto sicuro
            origin->discard_card(origin_card);
        } else if (target != origin) {
            origin->unequip_if_enabled(origin_card);
            target->equip_card(origin_card);
        }
        origin->m_game->pop_request<request_move_bomb>();
    }
}