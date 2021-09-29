#include "common/responses.h"
#include "common/effects.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void response_predraw::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_table) {
            card &c = target->get_table_card(card_id);
            if (target->is_top_predraw_check(c)) {
                auto t = target;
                t->get_game()->pop_response();
                for (auto &e : c.effects) {
                    e->on_predraw_check(t, &c);
                }
            }
        }
    }

    void response_draw::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::main_deck) {
            auto t = target;
            t->get_game()->pop_response();
            t->add_to_hand(target->get_game()->draw_card());
            t->add_to_hand(target->get_game()->draw_card());
        }
    }

    void response_check::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto t = target;
            t->get_game()->pop_response();
            t->get_game()->resolve_check(card_id);
        }
    }

    void response_generalstore::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::temp_table) {
            auto t = target;
            t->get_game()->pop_response();
            t->add_to_hand(t->get_game()->draw_from_temp(card_id));
        }
    }

    void response_discard::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto t = target;
            t->get_game()->pop_response();
            t->discard_card(&t->get_hand_card(card_id));
            if (t->num_hand_cards() <= t->get_hp()) {
                t->get_game()->next_turn();
            }
        }
    }
    
    void response_duel::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto &target_card = target->get_hand_card(card_id);
            if (!target_card.effects.empty() && target_card.effects.front().is<effect_bangcard>()) {
                auto t = target;
                t->get_game()->pop_response();
                t->discard_card(&target_card);
                t->get_game()->queue_response<response_type::duel>(target, origin);
            }
        }
    }

    void response_duel::on_resolve() {
        auto t = target;
        t->get_game()->pop_response();
        t->damage(origin, 1);
    }

    void response_indians::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto &target_card = target->get_hand_card(card_id);
            if (!target_card.effects.empty() && target_card.effects.front().is<effect_bangcard>()) {
                auto t = target;
                t->get_game()->pop_response();
                t->discard_card(&target_card);
            }
        }
    }

    void response_indians::on_resolve() {
        auto t = target;
        t->get_game()->pop_response();
        t->damage(origin, 1);
    }

    bool response_bang::on_respond(card *target_card) {
        if (!target_card->effects.empty()) {
            if (target_card->effects.front().is<effect_barrel>()) {
                target->get_game()->draw_check_then(target, [target = target](card_suit_type suit, card_value_type) {
                    if (suit == card_suit_type::hearts) {
                        target->get_game()->pop_response();
                    }
                });
                return true;
            } else if (target_card->effects.front().is<effect_missed>()) {
                target->get_game()->pop_response();
                return true;
            }
        }
        return false;
    }

    void response_bang::on_resolve() {
        auto t = target;
        t->get_game()->pop_response();
        t->damage(origin, 1);
    }

    bool response_death::on_respond(card *target_card) {
        if (!target_card->effects.empty()) {
            if (target_card->effects.front().is<effect_beer>()) {
                auto &moved = target->discard_card(target_card);
                for (auto &e : moved.effects) {
                    e->on_play(target);
                }
                if (target->get_hp() > 0) {
                    target->get_game()->pop_response();
                }
            }
        }
        return false;
    }

    void response_death::on_resolve() {
        auto t = target;
        t->get_game()->pop_response();
        t->get_game()->player_death(origin, target);
    }
}