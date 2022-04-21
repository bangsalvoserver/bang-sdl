#include "player.h"

#include "game.h"

#include "play_verify.h"

namespace banggame {
    using namespace enums::flag_operators;

    static std::vector<player *> make_equip_set(player *origin, card *card, target_player_filter filter) {
        std::vector<player *> ret;
        for (player &p : origin->m_game->m_players) {
            if (!check_player_filter(origin, filter, &p) && !p.find_equipped_card(card)) {
                ret.push_back(&p);
            }
        }
        return ret;
    }

    static target_list make_card_target_set(player *origin, card *origin_card, const effect_holder &holder) {
        target_list ret;

        for (player &target : origin->m_game->m_players) {
            if (!check_player_filter(origin, holder.player_filter, &target)) {
                if (holder.target == play_card_target_type::player) {
                    ret.emplace_back(target_player_t{&target});
                }
            } else {
                continue;
            }
            if (holder.target == play_card_target_type::card) {
                if (!bool(holder.card_filter & target_card_filter::hand)) {
                    for (card *target_card : target.m_table) {
                        if (target_card != origin_card
                            && (target_card->color == card_color_type::black) == bool(holder.card_filter & target_card_filter::black)
                        ) {
                            ret.emplace_back(target_card_t{target_card});
                        }
                    }
                }
                if (!bool(holder.card_filter & target_card_filter::table)) {
                    for (card *target_card : target.m_hand) {
                        if (target_card != origin_card) {
                            ret.emplace_back(target_card_t{target_card});
                        }
                    }
                }
            }
        }
        return ret;
    }

    bool player::is_possible_to_play(card *target_card) {
        if (target_card->color == card_color_type::brown) {
            switch (target_card->modifier) {
            case card_modifier_type::none:
                if (target_card->effects.empty()) return false;
                return std::ranges::all_of(target_card->effects, [&](const effect_holder &holder) {
                    return holder.target == play_card_target_type::none
                        || !make_card_target_set(this, target_card, holder).empty();
                });
            case card_modifier_type::bangmod: {
                effect_holder holder;
                holder.target = play_card_target_type::player;
                holder.player_filter = target_player_filter::reachable | target_player_filter::notself;
                return std::ranges::any_of(m_hand, [](card *c) {
                    return c->equips.empty() && c->owner->is_bangcard(c);
                }) && !make_card_target_set(this, target_card, holder).empty();
            }
            default: return true;
            }
        } else {
            if (m_game->has_scenario(scenario_flags::judge)) return false;
            if (!target_card->self_equippable()) {
                return !make_equip_set(this, target_card, target_card->equips.front().player_filter).empty();
            }
            return true;
        }
    }
}