#include "game/filters.h"

#include "player.h"
#include "game.h"

namespace banggame::filter_impl {
    
    int get_player_hp(player_view *origin) {
        return origin->hp;
    }

    bool check_player_flags(player_view *origin, player_flags flags) {
        return origin->has_player_flags(flags);
    }

    bool is_player_alive(player_view *origin) {
        return origin->alive();
    }

    player_role get_player_role(player_view *origin) {
        return origin->m_role.role;
    }

    int get_player_range_mod(player_view *origin) {
        return origin->m_range_mod;
    }

    int get_player_weapon_range(player_view *origin) {
        return origin->m_weapon_range;
    }

    int get_distance(player_view *origin, player_view *target) {
        return origin->m_game->get_target_finder().calc_distance(origin, target);
    }

    bool is_bangcard(player_view *origin, card_view *target) {
        return origin->m_game->get_target_finder().is_bangcard(target);
    }

    card_sign get_card_sign(player_view *origin, card_view *target) {
        return target->sign;
    }

    card_color_type get_card_color(card_view *target) {
        return target->color;
    }

    pocket_type get_card_pocket(card_view *target) {
        return target->pocket->type;
    }

    card_deck_type get_card_deck(card_view *target) {
        return target->deck;
    }

    bool is_cube_slot(card_view *target) {
        return target == target->pocket->owner->m_characters.front() || target->is_orange();
    }

    bool card_has_tag(card_view *target, tag_type type) {
        return target->has_tag(type);
    }

    bool allowed_with_leevankliff(player_view *origin, card_view *target) {
        return target->has_tag(tag_type::bangcard);
    }
}