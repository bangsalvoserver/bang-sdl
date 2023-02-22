#include "game/filters.h"

#include "player.h"

namespace banggame {

    using modifier_bitset_t = enums::sized_int_t<1 << (enums::num_members_v<card_modifier_type> - 1)>;

    constexpr modifier_bitset_t modifier_bitset(std::same_as<card_modifier_type> auto ... values) {
        return ((1 << enums::to_underlying(values)) | ... | 0);
    }

    inline modifier_bitset_t allowed_modifiers_after(card_modifier_type modifier) {
        switch (modifier) {
        case card_modifier_type::bangmod:
        case card_modifier_type::doublebarrel:
        case card_modifier_type::bandolier:
            return modifier_bitset(card_modifier_type::bangmod, card_modifier_type::doublebarrel, card_modifier_type::bandolier);
        case card_modifier_type::discount:
            return modifier_bitset(card_modifier_type::shopchoice);
        case card_modifier_type::shopchoice:
        case card_modifier_type::leevankliff:
            return modifier_bitset();
        default:
            return ~(-1 << enums::num_members_v<card_modifier_type>);
        }
    }

    inline bool allowed_card_with_modifier(player_view *origin, card_view *mod_card, card_view *target) {
        switch (mod_card->modifier_type()) {
        case card_modifier_type::bangmod:
        case card_modifier_type::doublebarrel:
        case card_modifier_type::bandolier:
            if (target->pocket->type == pocket_type::player_hand) {
                return target->has_tag(tag_type::bangcard);
            } else {
                return target->has_tag(tag_type::play_as_bang);
            }
        case card_modifier_type::leevankliff:
            return origin->m_game->get_target_finder().get_last_played_card() == target && target->is_brown();
        case card_modifier_type::discount:
            return target->deck == card_deck_type::goldrush && target->pocket->type != pocket_type::player_table;
        case card_modifier_type::shopchoice:
            return target->deck == card_deck_type::goldrush && target->pocket->type == pocket_type::hidden_deck
                && mod_card->get_tag_value(tag_type::shopchoice) == target->get_tag_value(tag_type::shopchoice);
        case card_modifier_type::belltower:
            return (target->pocket->type != pocket_type::player_hand && target->pocket->type != pocket_type::shop_selection)
                || target->is_brown();
        default:
            return true;
        }
    }

} 