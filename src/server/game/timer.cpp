#include "common/timer.h"

#include "player.h"
#include "game.h"

namespace banggame {
    void timer_damaging::on_finished() {
        target->m_hp -= damage;
        target->m_game->add_public_update<game_update_type::player_hp>(target->id, target->m_hp);
        if (target->m_hp <= 0) {
            target->m_game->pop_request_noupdate();
            target->m_game->add_request<request_type::death>(0, origin, target);
        } else {
            target->m_game->pop_request();
        }
        target->m_game->queue_event<event_type::on_hit>(origin, target, damage, is_bang);
    }
}