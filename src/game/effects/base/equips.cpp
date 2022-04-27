#include "equips.h"

#include "../../game.h"

namespace banggame {

    void event_based_effect::on_disable(card *target_card, player *target) {
        target->m_game->remove_events(target_card);
    }

    void predraw_check_effect::on_disable(card *target_card, player *target) {
        target->remove_predraw_check(target_card);
    }

    void effect_max_hp::on_enable(card *target_card, player *target) {
        if (target_card == target->m_characters.front()) {
            target->m_max_hp = value + (target->m_role == player_role::sheriff);
        }
    }

    void effect_mustang::on_enable(card *target_card, player *target) {
        ++target->m_distance_mod;
        target->send_player_status();
    }

    void effect_mustang::on_disable(card *target_card, player *target) {
        --target->m_distance_mod;
        target->send_player_status();
    }

    void effect_scope::on_enable(card *target_card, player *target) {
        ++target->m_range_mod;
        target->send_player_status();
    }

    void effect_scope::on_disable(card *target_card, player *target) {
        --target->m_range_mod;
        target->send_player_status();
    }

    opt_fmt_str effect_prompt_on_self_equip::on_prompt(card *target_card, player *target) const {
        if (target == target_card->owner) {
            return game_formatted_string{"PROMPT_EQUIP_ON_SELF", target_card};
        } else {
            return std::nullopt;
        }
    }

    void effect_jail::on_enable(card *target_card, player *target) {
        target->add_predraw_check(target_card, 1, [=](card *drawn_card) {
            target->discard_card(target_card);
            if (target->get_card_sign(drawn_card).suit == card_suit::hearts) {
                target->m_game->add_log("LOG_JAIL_BREAK", target);
                target->next_predraw_check(target_card);
            } else {
                target->m_game->add_log("LOG_SKIP_TURN", target);
                target->skip_turn();
            }
        });
    }

    void effect_dynamite::on_enable(card *target_card, player *target) {
        target->add_predraw_check(target_card, 2, [=](card *drawn_card) {
            card_sign sign = target->get_card_sign(drawn_card);
            if (sign.suit == card_suit::spades
                && enums::indexof(sign.rank) >= enums::indexof(card_rank::rank_2)
                && enums::indexof(sign.rank) <= enums::indexof(card_rank::rank_9)) {
                target->m_game->add_log("LOG_CARD_EXPLODES", target_card);
                target->discard_card(target_card);
                target->damage(target_card, nullptr, 3);
            } else {
                auto *p = target;
                do {
                    p = p->m_game->get_next_player(p);
                } while (p->find_equipped_card(target_card) && p != target);

                if (p != target) {
                    target_card->on_disable(target);
                    target_card->on_equip(p);
                    p->equip_card(target_card);
                }
            }
            target->next_predraw_check(target_card);
        });
    }

    static bool is_horse(const card *c) {
        return c->equips.first_is(equip_type::horse);
    }

    opt_fmt_str effect_horse::on_prompt(card *target_card, player *target) const {
        if (auto it = std::ranges::find_if(target->m_table, is_horse); it != target->m_table.end()) {
            return game_formatted_string{"PROMPT_REPLACE", target_card, *it};
        } else {
            return std::nullopt;
        }
    }

    void effect_horse::on_equip(card *target_card, player *target) {
        if (auto it = std::ranges::find_if(target->m_table, is_horse); it != target->m_table.end()) {
            target->discard_card(*it);
        }
    }

    opt_fmt_str effect_weapon::on_prompt(card *target_card, player *target) const {
        if (target == target_card->owner) {
            if (auto it = std::ranges::find_if(target->m_table, &card::is_weapon); it != target->m_table.end()) {
                return game_formatted_string{"PROMPT_REPLACE", target_card, *it};
            }
        }
        return std::nullopt;
    }

    void effect_weapon::on_equip(card *target_card, player *target) {
        if (auto it = std::ranges::find_if(target->m_table, &card::is_weapon); it != target->m_table.end()) {
            target->m_game->add_log("LOG_DISCARDED_SELF_CARD", target, *it);
            target->discard_card(*it);
        }
    }

    void effect_weapon::on_enable(card *target_card, player *target) {
        target->m_weapon_range = range;
        target->send_player_status();
    }

    void effect_weapon::on_disable(card *target_card, player *target) {
        target->m_weapon_range = 1;
        target->send_player_status();
    }

    void effect_volcanic::on_enable(card *target_card, player *target) {
        target->m_game->add_event<event_type::apply_volcanic_modifier>(target_card, [=](player *p, bool &value) {
            value = value || p == target;
        });
    }

    void effect_boots::on_enable(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>({target_card, 1}, [p, target_card](card *origin_card, player *origin, player *target, int damage, bool is_bang){
            if (p == target) {
                target->m_game->queue_action([=]{
                    if (target->alive()) {
                        target->draw_card(damage, target_card);
                    }
                });
            }
        });
    }

    void effect_horsecharm::on_enable(card *target_card, player *target) {
        ++target->m_num_checks;
    }

    void effect_horsecharm::on_disable(card *target_card, player *target) {
        --target->m_num_checks;
    }
}