#include "common/scenarios.h"

#include "player.h"
#include "game.h"

namespace banggame {
    using namespace enums::flag_operators;

    void effect_blessing::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::apply_suit_modifier>(target_card, [](card_suit_type &suit) {
            suit = card_suit_type::hearts;
        });
    }

    void effect_curse::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::apply_suit_modifier>(target_card, [](card_suit_type &suit) {
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
        target->m_game->add_event<event_type::pre_turn_start>(target_card, [=](player *p) {
            p->damage(target_card, nullptr, 1);
        });
    }

    void effect_shootout::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::on_turn_start>(target_card, [](player *p) {
            ++p->m_bangs_per_turn;
        });
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
        target->m_game->add_event<event_type::pre_turn_start>(target_card, [=](player *p) {
            for (int i=0; i<p->m_hand.size(); ++i) {
                p->m_game->queue_request<request_type::bang>(target_card, nullptr, p);
            }
        });
    }

    void effect_judge::on_equip(player *target, card *target_card) {
        target->m_game->m_scenario_flags |= scenario_flags::judge;
    }

    void effect_peyote::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::on_turn_start>(target_card, [=](player *p) {
            auto &vec = p->m_game->m_hidden_deck;
            for (auto it = vec.begin(); it != vec.end(); ) {
                auto *card = *it;
                if (!card->responses.empty() && card->responses.front().is(effect_type::peyotechoice)) {
                    it = p->m_game->move_to(card, card_pile_type::selection, true, nullptr, show_card_flags::no_animation);
                } else {
                    ++it;
                }
            }

            p->m_has_drawn = true;
            p->m_game->queue_request<request_type::peyote>(target_card, nullptr, p);
        });
    }

    void request_peyote::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        auto *drawn_card = target->m_game->m_deck.back();
        target->m_game->send_card_update(*drawn_card, nullptr, show_card_flags::short_pause);

        if ((target_card->responses.front().args == 1)
            ? (drawn_card->suit == card_suit_type::hearts || drawn_card->suit == card_suit_type::diamonds)
            : (drawn_card->suit == card_suit_type::clubs || drawn_card->suit == card_suit_type::spades))
        {
            target->m_game->draw_card_to(card_pile_type::player_hand, target);
        } else {
            target->m_game->draw_card_to(card_pile_type::discard_pile);

            while (!target->m_game->m_selection.empty()) {
                target->m_game->move_to(target->m_game->m_selection.front(), card_pile_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
            }
            target->m_game->pop_request();
        }
    }

    void effect_handcuffs::on_equip(player *target, card *target_card) {
        target->m_game->add_event<event_type::post_draw_cards>(target_card, [=](player *origin) {
            auto &vec = origin->m_game->m_hidden_deck;
            for (auto it = vec.begin(); it != vec.end(); ) {
                auto *card = *it;
                if (!card->responses.empty() && card->responses.front().is(effect_type::handcuffschoice)) {
                    it = origin->m_game->move_to(card, card_pile_type::selection, true, nullptr, show_card_flags::no_animation);
                } else {
                    ++it;
                }
            }
            origin->m_game->queue_request<request_type::handcuffs>(target_card, nullptr, origin);
        });
    }

    void request_handcuffs::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->m_declared_suit = static_cast<card_suit_type>(target_card->responses.front().args);

        auto &vec = target->m_game->m_selection;
        for (auto it = vec.begin(); it != vec.end();) {
            if (*it != target_card) {
                show_card_flags flags = show_card_flags::no_animation;
                if (vec.size() == 2) {
                    flags |= show_card_flags::short_pause;
                }
                it = target->m_game->move_to(*it, card_pile_type::hidden_deck, true, nullptr, flags);
            } else {
                ++it;
            }
        }
        target->m_game->move_to(vec.front(), card_pile_type::hidden_deck, true, nullptr, show_card_flags::no_animation);
        target->m_game->pop_request();
    }
}