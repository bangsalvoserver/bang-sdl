#include "common/responses.h"
#include "common/effects.h"
#include "common/net_enums.h"

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
            t->add_to_hand(t->get_game()->draw_card());
            t->add_to_hand(t->get_game()->draw_card());
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
                auto o = origin;
                auto t = target;
                t->get_game()->pop_response();
                t->discard_card(&target_card);
                t->get_game()->queue_response<response_type::duel>(t, o);
            }
        }
    }

    void response_duel::on_resolve() {
        auto o = origin;
        auto t = target;
        t->get_game()->pop_response();
        t->damage(o, 1);
    }

    void response_indians::on_pick(card_pile_type pile, int card_id) {
        if (pile == card_pile_type::player_hand) {
            auto t = target;
            auto &target_card = t->get_hand_card(card_id);
            if (!target_card.effects.empty() && target_card.effects.front().is<effect_bangcard>()) {
                t->get_game()->pop_response();
                t->discard_card(&target_card);
            }
        }
    }

    void response_indians::on_resolve() {
        auto o = origin;
        auto t = target;
        t->get_game()->pop_response();
        t->damage(o, 1);
    }
    
    void response_bang::handle_missed() {
        if (0 == --get_data()->bang_strength) {
            target->get_game()->pop_response();
        }
    }

    bool response_bang::on_respond(card *target_card) {
        if (!target_card->effects.empty()) {
            if (target_card->effects.front().is<effect_barrel>()) {
                auto &vec = get_data()->barrels_used;
                if (std::ranges::find(vec, target_card->id) != std::ranges::end(vec)) {
                    return false;
                }
                auto pos = std::ranges::find(vec, 0);
                if (pos == std::ranges::end(vec)) {
                    throw game_error("Risposto con troppi barili");
                }
                *pos = target_card->id;
                target->get_game()->draw_check_then(target, [this](card_suit_type suit, card_value_type) {
                    if (suit == card_suit_type::hearts) {
                        handle_missed();
                    }
                });
                return true;
            } else if (target_card->effects.front().is<effect_missed>()) {
                handle_missed();
                return true;
            }
        }
        return false;
    }

    void response_bang::on_resolve() {
        auto o = origin;
        auto t = target;
        t->get_game()->pop_response();
        t->damage(o, 1);
    }

    bool response_death::on_respond(card *target_card) {
        if (!target_card->effects.empty()) {
            if (target_card->effects.front().is<effect_deathsave>()) {
                if (target->get_hp() == 0) {
                    target->get_game()->pop_response();
                }
                return true;
            }
        }
        return false;
    }

    void response_death::on_resolve() {
        auto o = origin;
        auto t = target;
        t->get_game()->pop_response();
        t->get_game()->player_death(o, t);
    }
}