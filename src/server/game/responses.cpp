#include "common/responses.h"
#include "common/effects.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void response_draw::on_pick(card_pile_type pile, int index) {
        if (pile == card_pile_type::main_deck) {
            target->add_to_hand(target->get_game()->draw_card());
            target->add_to_hand(target->get_game()->draw_card());
            target->get_game()->pop_response();
        }
    }

    void response_check::on_pick(card_pile_type pile, int index) {
        if (pile == card_pile_type::temp_table) {
            target->get_game()->resolve_check(index);
            target->get_game()->pop_response();
        }
    }

    void response_generalstore::on_pick(card_pile_type pile, int index) {
        if (pile == card_pile_type::temp_table) {
            target->add_to_hand(target->get_game()->draw_from_temp(index));
            target->get_game()->pop_response();
        }
    }

    void response_discard::on_pick(card_pile_type pile, int index) {
        if (pile == card_pile_type::player_hand) {
            target->discard_hand_card(index);
            target->get_game()->pop_response();
        }
    }

    bool response_bang::on_respond(card *target_card) {
        if (!target_card->effects.empty()) {
            if (target_card->effects.front().is<effect_barrel>()) {
                target->get_game()->draw_check_then(target, [&](card *c) {
                    if (c->suit == card_suit_type::hearts) {
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
        target->damage(1);
        target->get_game()->pop_response();
    }
    
    bool response_duel::on_respond(card *target_card) {
        if (!target_card->effects.empty()) {
            if (target_card->effects.front().is<effect_bangcard>()) {
                target->get_game()->pop_response();
                target->get_game()->add_response<response_type::duel>(target, origin);
                return true;
            }
        }
        return false;
    }

    void response_duel::on_resolve() {
        target->damage(1);
        target->get_game()->pop_response();
    }

    bool response_indians::on_respond(card *target_card) {
        if (!target_card->effects.empty()) {
            if (target_card->effects.front().is<effect_bangcard>()) {
                target->get_game()->pop_response();
                return true;
            }
        }
        return false;
    }

    void response_indians::on_resolve() {
        target->damage(1);
        target->get_game()->pop_response();
    }

    bool response_death::on_respond(card *target_card) {
        if (!target_card->effects.empty()) {
            if (target_card->effects.front().is<effect_beer>()) {
                if (target->get_game()->num_alive() > 2 && target->get_hp() == 0) {
                    target->get_game()->pop_response();
                    return true;
                }
            }
        }
        return false;
    }

    void response_death::on_resolve() {
        target->get_game()->player_death(target);
        target->get_game()->pop_response();
    }
}