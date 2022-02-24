#include "player.h"

#include "game.h"

#include "holders.h"

#include <cassert>

namespace banggame {
    using namespace enums::flag_operators;

    bool valid_player(player *origin, player *target, target_player_filter filter) {
        if (!target->alive() && !bool(filter & target_player_filter::dead)) return false;
        return std::ranges::all_of(util::enum_flag_values(filter), [&](target_player_filter value) {
            switch (value) {
            case target_player_filter::self: return origin == target;
            case target_player_filter::notself: return origin != target;
            case target_player_filter::notsheriff: return target->m_role != player_role::sheriff;
            case target_player_filter::reachable:
                return origin->m_weapon_range > 0
                    && origin->m_game->calc_distance(origin, target) <= origin->m_weapon_range + origin->m_range_mod;
            case target_player_filter::range_1: return origin->m_game->calc_distance(origin, target) <= 1 + origin->m_range_mod;
            case target_player_filter::range_2: return origin->m_game->calc_distance(origin, target) <= 2 + origin->m_range_mod;
            default: return true;
            }
        });
    }

    std::vector<player *> make_equip_set(player *origin, card *card, target_player_filter filter) {
        std::vector<player *> ret;
        for (player &p : origin->m_game->m_players) {
            if (valid_player(origin, &p, filter) && !p.find_equipped_card(card)) {
                ret.push_back(&p);
            }
        }
        return ret;
    }

    struct target_pair {
        player *target_player;
        card *target_card;
    };

    std::vector<target_pair> make_card_target_set(player *origin, play_card_target_type type, target_player_filter player_filter, target_card_filter card_filter) {
        std::vector<target_pair> ret;

        for (player &target : origin->m_game->m_players) {
            if (valid_player(origin, &target, player_filter)) {
                if (type == play_card_target_type::player) {
                    ret.emplace_back(&target, nullptr);
                }
            } else {
                continue;
            }
            if (type == play_card_target_type::card) {
                if (!bool(card_filter & target_card_filter::hand)) {
                    for (card *target_card : target.m_table) {
                        if ((target_card->color == card_color_type::black) == bool(card_filter & target_card_filter::black)) {
                            ret.emplace_back(&target, target_card);
                        }
                    }
                }
                if (!bool(card_filter & target_card_filter::table)) {
                    for (card *target_card : target.m_hand) {
                        ret.emplace_back(&target, target_card);
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
                        || !make_card_target_set(this, holder.target, holder.player_filter, holder.card_filter).empty();
                });
            case card_modifier_type::bangcard: {
                return (std::ranges::any_of(m_hand, [](card *c) {
                    return std::ranges::find(c->effects, effect_type::bangcard, &effect_holder::type) != c->effects.end();
                })) && !make_card_target_set(this,
                    play_card_target_type::player,
                    target_player_filter::reachable | target_player_filter::notself,
                    enums::flags_none<target_card_filter>).empty();
            }
            default: return true;
            }
        } else {
            if (m_game->has_scenario(scenario_flags::judge)) return false;
            if (!target_card->equips.empty() && target_card->equips.front().target != play_card_target_type::none) {
                return !make_equip_set(this, target_card, target_card->equips.front().player_filter).empty();
            }
            return true;
        }
    }
}