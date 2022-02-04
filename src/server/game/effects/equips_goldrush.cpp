#include "common/effects/equips_goldrush.h"

#include "../game.h"

namespace banggame {

    void effect_luckycharm::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>(target_card, [p](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (p == target) {
                target->add_gold(damage);
            }
        });
    }

    void effect_pickaxe::on_equip(card *target_card, player *target) {
        ++target->m_num_cards_to_draw;
    }

    void effect_pickaxe::on_unequip(card *target_card, player *target) {
        --target->m_num_cards_to_draw;
    }

    void effect_calumet::on_equip(card *target_card, player *target) {
        ++target->m_calumets;
    }

    void effect_calumet::on_unequip(card *target_card, player *target) {
        --target->m_calumets;
    }

    void effect_gunbelt::on_equip(card *target_card, player *target) {
        target->m_max_cards_mods.push_back(args);
    }

    void effect_gunbelt::on_unequip(card *target_card, player *target) {
        auto it = std::ranges::find(target->m_max_cards_mods, args);
        if (it != target->m_max_cards_mods.end()) {
            target->m_max_cards_mods.erase(it);
        }
    }

    void effect_wanted::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_player_death>(target_card, [p](player *origin, player *target) {
            if (origin && p == target && origin != target) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
                origin->add_gold(1);
            }
        });
    }

}