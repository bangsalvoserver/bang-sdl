#include "equips.h"
#include "requests.h"

#include "../base/requests.h"

#include "../../game.h"

namespace banggame {

    void effect_snake::on_equip(card *target_card, player *target) {
        target->add_predraw_check(target_card, 0, [=](card *drawn_card) {
            if (target->get_card_sign(drawn_card).suit == card_suit::spades) {
                target->damage(target_card, nullptr, 1);
            }
            target->next_predraw_check(target_card);
        });
    }

    void effect_ghost::on_pre_equip(card *target_card, player *target) {
        for (card *c : target->m_characters) {
            target->equip_if_enabled(c);
        }
        target->m_game->add_event<event_type::post_discard_card>(target_card, [=](player *p, card *c) {
            if (p == target && c == target_card) {
                target->m_game->queue_action_front([=]{
                    target->remove_player_flags(player_flags::ghost);
                    target->m_game->player_death(nullptr, target);
                    target->m_game->check_game_over(nullptr, target);
                });
                target->m_game->remove_events(target_card);
            }
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
        p->m_game->add_event<event_type::on_hit>({target_card, 4}, [=](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin == p && target != p && is_bang) {
                target->m_game->queue_action([=]{
                    if (target->alive() && !target->m_hand.empty()) {
                        target->m_game->queue_request<request_discard>(target_card, origin, target);
                    }
                });
            }
        });
    }

    void effect_bounty::on_equip(card *target_card, player *p) {
        p->m_game->add_event<event_type::on_hit>({target_card, 3}, [p](card *origin_card, player *origin, player *target, int damage, bool is_bang) {
            if (origin && target == p && is_bang) {
                origin->m_game->draw_card_to(pocket_type::player_hand, origin);
            }
        });
    }
}