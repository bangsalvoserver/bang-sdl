#include "player.h"

#include "game.h"

#include "holders.h"

#include <cassert>

namespace banggame {
    using namespace enums::flag_operators;

    std::vector<player *> make_equip_set(player *origin, card *card, target_type type) {
        std::vector<player *> ret;
        for (player &p : origin->m_game->m_players) {
            if (std::ranges::all_of(util::enum_flag_values(type), [&](target_type value) {
                switch (value) {
                case target_type::player: return p.alive() || bool(type & target_type::dead);
                case target_type::dead: return p.m_hp == 0;
                case target_type::notsheriff: return p.m_role != player_role::sheriff;
                default: return false;
                }
            }) && !p.find_equipped_card(card)) {
                ret.push_back(&p);
            }
        }
        return ret;
    }

    struct target_pair {
        player *target_player;
        card *target_card;
    };

    std::vector<target_pair> make_card_target_set(player *origin, target_type type) {
        std::vector<target_pair> ret;

        for (player &target : origin->m_game->m_players) {
            if (std::ranges::all_of(util::enum_flag_values(type), [origin, target = &target](target_type value) {
                switch (value) {
                case target_type::player: return target->alive();
                case target_type::self: return origin == target;
                case target_type::notself: return origin != target;
                case target_type::reachable:
                    return origin->m_weapon_range > 0
                        && origin->m_game->calc_distance(origin, target) <= origin->m_weapon_range + origin->m_range_mod;
                case target_type::range_1: return origin->m_game->calc_distance(origin, target) <= 1 + origin->m_range_mod;
                case target_type::range_2: return origin->m_game->calc_distance(origin, target) <= 2 + origin->m_range_mod;
                case target_type::fanning_target: // TODO
                default: return true;
                }
            })) {
                if (bool(type & target_type::player)) {
                    ret.emplace_back(&target, nullptr);
                }
            } else {
                continue;
            }
            if (bool(type & target_type::card)) {
                if (!bool(type & target_type::hand)) {
                    for (card *target_card : target.m_table) {
                        if ((target_card->color == card_color_type::black) == bool(type & target_type::black)) {
                            ret.emplace_back(&target, target_card);
                        }
                    }
                }
                if (!bool(type & target_type::table)) {
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
                return std::ranges::all_of(target_card->effects, [&](target_type type) {
                    return type == enums::flags_none<target_type>
                        || !make_card_target_set(this, type).empty();
                }, &effect_holder::target);
            case card_modifier_type::bangcard: {
                return (std::ranges::any_of(m_hand, [](card *c) {
                    return std::ranges::find(c->effects, effect_type::bangcard, &effect_holder::type) != c->effects.end();
                })) && !make_card_target_set(this, target_type::player | target_type::reachable | target_type::notself).empty();
            }
            default: return true;
            }
        } else {
            if (m_game->has_scenario(scenario_flags::judge)) return false;
            if (!target_card->equips.empty() && target_card->equips.front().target != enums::flags_none<target_type>) {
                return !make_equip_set(this, target_card, target_card->equips.front().target).empty();
            }
            return true;
        }
    }
}