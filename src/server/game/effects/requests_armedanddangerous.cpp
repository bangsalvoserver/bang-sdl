#include "common/effects/requests_armedanddangerous.h"

#include "../game.h"

namespace banggame {

    bool request_add_cube::can_pick(card_pile_type pile, player *target_player, card *target_card) const {
        return ((pile == card_pile_type::player_character && target_card == target->m_characters.front())
            || (pile == card_pile_type::player_table && target_card->color == card_color_type::orange))
            && target_player == target && target_card->cubes.size() < 4;
    }

    void request_add_cube::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        target->add_cubes(target_card, 1);
        if (--target->m_game->top_request().get<request_type::add_cube>().ncubes == 0 || !target->can_receive_cubes()) {
            target->m_game->pop_request(request_type::add_cube);
        }
    }

    game_formatted_string request_add_cube::status_text() const {
        if (origin_card) {
            return {"STATUS_ADD_CUBE_FOR", origin_card};
        } else {
            return "STATUS_ADD_CUBE";
        }
    }

    void request_move_bomb::on_pick(card_pile_type pile, player *target_player, card *target_card) {
        if (!target_player->immune_to(origin_card)) {
            if (target_player == target) {
                target->m_game->pop_request(request_type::move_bomb);
            } else if (!target_player->find_equipped_card(origin_card)) {
                origin_card->on_unequip(target);
                target_player->equip_card(origin_card);
                target->m_game->pop_request(request_type::move_bomb);
            }
        } else {
            target->discard_card(origin_card);
            target->m_game->pop_request(request_type::move_bomb);
        }
    }

    game_formatted_string request_move_bomb::status_text() const {
        return {"STATUS_MOVE_BOMB", origin_card};
    }

    void request_rust::on_resolve() {
        target->m_game->pop_request(request_type::rust);

        effect_rust{}.on_play(origin_card, origin, target);
    }

    game_formatted_string request_rust::status_text() const {
        return {"STATUS_RUST", origin_card};
    }

    game_formatted_string timer_al_preacher::status_text() const {
        return {"STATUS_CAN_PLAY_CARD", origin_card};
    }

    game_formatted_string timer_tumbleweed::status_text() const {
        return {"STATUS_CAN_PLAY_TUMBLEWEED", target->m_game->m_current_check->origin, origin_card, target_card, drawn_card};
    }

}