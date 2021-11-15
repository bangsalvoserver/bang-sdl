#include "common/scenarios.h"

#include "player.h"
#include "game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_blessing::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::apply_check_modifier>(target_card, [](card_suit_type &suit, card_value_type &value) {
            suit = card_suit_type::hearts;
        });
    }

    void effect_curse::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::apply_check_modifier>(target_card, [](card_suit_type &suit, card_value_type &value) {
            suit = card_suit_type::spades;
        });
    }

    void effect_thedaltons::on_equip(player *target, card *target_card) {
        player *p = target;
        while(true) {
            if (std::ranges::find(p->m_table, card_color_type::blue, &card::color) != p->m_table.end()) {
                p->m_game->queue_request<request_type::thedaltons>(target_card, nullptr, p);
            }

            p = target->m_game->get_next_player(p);
            if (p == target) break;
        }
    }

    void request_thedaltons::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (target_card->color == card_color_type::blue && target == target_player) {
            target->discard_card(target_card);
            target->m_game->pop_request();
        }
    }

    void effect_thedoctor::on_equip(player *target, card *target_card) {
        int min_hp = std::ranges::min(target->m_game->m_players
            | std::views::filter(&player::alive)
            | std::views::transform(&player::m_hp));
        
        for (auto &p : target->m_game->m_players) {
            if (p.m_hp == min_hp) {
                p.heal(1);
            }
        }
    }

    void effect_trainarrival::on_equip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            ++p.m_num_cards_to_draw;
        }
    }

    void effect_trainarrival::on_unequip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            --p.m_num_cards_to_draw;
        }
    }

    void effect_thirst::on_equip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            --p.m_num_cards_to_draw;
        }
    }

    void effect_thirst::on_unequip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            ++p.m_num_cards_to_draw;
        }
    }

    void effect_highnoon::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::on_turn_start>(target_card, [=](player *p) {
            p->damage(target_card, nullptr, 1);
        });
    }

    void effect_shootout::on_equip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            ++p.m_bangs_per_turn;
        }
    }

    void effect_shootout::on_unequip(player *target, card *target_card) {
        for (auto &p : target->m_game->m_players) {
            p.m_bangs_per_turn = 0;
        }
    }

    void effect_invert_rotation::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::invert_rotation;
    }

    void effect_reverend::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::reverend;
    }

    void effect_hangover::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::hangover;
        target->m_game->disable_characters();
    }

    void effect_hangover::on_unequip(player *target, card *target_card) {
        target->m_game->enable_characters();
        target->m_game->queue_event<event_type::on_effect_end>(target, target_card);
    }

    void effect_sermon::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::sermon;
    }

    void effect_ghosttown::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::ghosttown;
    }

    void effect_ambush::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::ambush;
    }

    void effect_lasso::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::lasso;
        target->m_game->disable_table_cards();
    }

    void effect_lasso::on_unequip(player *target, card *target_card) {
        target->m_game->enable_table_cards();
    }

    void effect_fistfulofcards::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::on_turn_start>(target_card, [=](player *p) {
            for (int i=0; i<p->m_hand.size(); ++i) {
                p->m_game->queue_request<request_type::bang>(target_card, nullptr, p);
            }
        });
    }

    void effect_judge::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::judge;
    }
}