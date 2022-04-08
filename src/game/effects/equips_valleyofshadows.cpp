#include "equips_valleyofshadows.h"
#include "requests_valleyofshadows.h"
#include "requests_base.h"

#include "../game.h"

namespace banggame {

    void effect_snake::on_equip(card *target_card, player *target) {
        target->add_predraw_check(target_card, 0, [=](card *drawn_card) {
            if (target->get_card_suit(drawn_card) == card_suit_type::spades) {
                target->damage(target_card, nullptr, 1);
            }
            target->next_predraw_check(target_card);
        });
    }

    void effect_ghost::on_pre_equip(card *target_card, player *target) {
        for (card *c : target->m_characters) {
            target->equip_if_enabled(c);
        }
        target->m_game->add_call_once_event<event_type::post_discard_card>(target_card, [=](player *p, card *c) {
            if (p == target && c == target_card) {
                target->m_game->queue_action([=]{
                    target->remove_player_flags(player_flags::ghost);
                    target->m_game->player_death(nullptr, target);
                    target->m_game->check_game_over(nullptr, target);
                });
                return true;
            }
            return false;
        });
    }

    void effect_ghost::on_equip(card *target_card, player *target) {
        target->m_game->add_public_update<game_update_type::player_hp>(target->id, 0, false);
        target->add_player_flags(player_flags::ghost);
    }

    void effect_ghost::on_unequip(card *target_card, player *target) {
        target->m_game->add_public_update<game_update_type::player_hp>(target->id, 0, true);
        target->remove_player_flags(player_flags::ghost);
    }

    void effect_shotgun::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>({target_card, 1}, [=](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin == p && target != p && !target->m_hand.empty() && is_bang) {
                target->m_game->queue_request<request_discard>(target_card, origin, target);
            }
        });
    }

    void effect_bounty::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>({target_card, 4}, [p](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin && target == p && is_bang) {
                origin->m_game->draw_card_to(card_pile_type::player_hand, origin);
            }
        });
    }
}