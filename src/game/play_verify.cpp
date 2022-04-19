#include "play_verify.h"

#include "player.h"
#include "game.h"

#include "effects/base/requests.h"

namespace banggame {
    using namespace enums::flag_operators;

    void modifier_play_card_verify::verify_modifiers() {
        for (card *mod_card : modifiers) {
            if (origin->m_game->is_disabled(mod_card)) {
                throw game_error("ERROR_CARD_IS_DISABLED", mod_card);
            }
            if (mod_card->modifier != card_modifier_type::bangmod || !card_ptr->effects.last_is(effect_type::bangcard)) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            for (const auto &e : mod_card->effects) {
                e.verify(mod_card, origin);
            }
        }
    }

    void modifier_play_card_verify::play_modifiers() const {
        for (card *mod_card : modifiers) {
            play_card_verify{origin, mod_card, false,
                {mod_card->effects.size(), target_none{}}}.do_play_card();
        }
    }

    void play_card_verify::verify_equip_target() {
        if (origin->m_game->is_disabled(card_ptr)) {
            throw game_error("ERROR_CARD_IS_DISABLED", card_ptr);
        }
        if (!card_ptr->equips.empty()) {
            if (targets.size() != 1 || !std::holds_alternative<target_player>(targets.front())) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            verify_effect_player_target(card_ptr->equips.front().player_filter, std::get<target_player>(targets.front()).target);
        }
    }

    void play_card_verify::verify_effect_player_target(target_player_filter filter, player *target) {
        if (target->alive() == bool(filter & target_player_filter::dead)) {
            throw game_error("ERROR_TARGET_DEAD");
        }

        std::ranges::for_each(enums::enum_flag_values(filter), [&](target_player_filter value) {
            switch (value) {
            case target_player_filter::self:
                if (target != origin) {
                    throw game_error("ERROR_TARGET_NOT_SELF");
                }
                break;
            case target_player_filter::notself:
                if (target == origin) {
                    throw game_error("ERROR_TARGET_SELF");
                }
                break;
            case target_player_filter::notsheriff:
                if (target->m_role == player_role::sheriff) {
                    throw game_error("ERROR_TARGET_SHERIFF");
                }
                break;
            case target_player_filter::reachable:
                if (!origin->m_weapon_range || origin->m_game->calc_distance(origin, target) > origin->m_weapon_range + origin->m_range_mod) {
                    throw game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::range_1:
                if (origin->m_game->calc_distance(origin, target) > 1 + origin->m_range_mod) {
                    throw game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::range_2:
                if (origin->m_game->calc_distance(origin, target) > 2 + origin->m_range_mod) {
                    throw game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::dead:
                break;
            default: throw game_error("ERROR_INVALID_ACTION");
            }
        });
    }

    void play_card_verify::verify_effect_card_target(const effect_holder &effect, player *target, card *target_card) {
        verify_effect_player_target(effect.player_filter, target);

        if ((target_card->color == card_color_type::black) != bool(effect.card_filter & target_card_filter::black)) {
            throw game_error("ERROR_TARGET_BLACK_CARD");
        }

        std::ranges::for_each(enums::enum_flag_values(effect.card_filter), [&](target_card_filter value) {
            switch (value) {
            case target_card_filter::table:
                if (target_card->pocket != pocket_type::player_table) {
                    throw game_error("ERROR_TARGET_NOT_TABLE_CARD");
                }
                break;
            case target_card_filter::hand:
                if (target_card->pocket != pocket_type::player_hand) {
                    throw game_error("ERROR_TARGET_NOT_HAND_CARD");
                }
                break;
            case target_card_filter::blue:
                if (target_card->color != card_color_type::blue) {
                    throw game_error("ERROR_TARGET_NOT_BLUE_CARD");
                }
                break;
            case target_card_filter::clubs:
                if (origin->get_card_sign(target_card).suit != card_suit::clubs) {
                    throw game_error("ERROR_TARGET_NOT_CLUBS");
                }
                break;
            case target_card_filter::bang:
                if (!origin->is_bangcard(target_card) || !target_card->equips.empty()) {
                    throw game_error("ERROR_TARGET_NOT_BANG");
                }
                break;
            case target_card_filter::missed:
                if (!target_card->responses.last_is(effect_type::missedcard)) {
                    throw game_error("ERROR_TARGET_NOT_MISSED");
                }
                break;
            case target_card_filter::beer:
                if (!target_card->effects.first_is(effect_type::beer)) {
                    throw game_error("ERROR_TARGET_NOT_BEER");
                }
                break;
            case target_card_filter::bronco:
                if (!target_card->equips.last_is(equip_type::bronco)) {
                    throw game_error("ERROR_TARGET_NOT_BRONCO");
                }
                break;
            case target_card_filter::cube_slot:
            case target_card_filter::cube_slot_card:
                if (target_card != target->m_characters.front() && target_card->color != card_color_type::orange)
                    throw game_error("ERROR_TARGET_NOT_CUBE_SLOT");
                break;
            case target_card_filter::can_repeat:
            case target_card_filter::black:
                break;
            default: throw game_error("ERROR_INVALID_ACTION");
            }
        });
    }

    void play_card_verify::verify_card_targets() {
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;

        if (origin->m_mandatory_card && origin->m_mandatory_card != card_ptr
            && std::ranges::find(origin->m_mandatory_card->effects, effect_type::banglimit, &effect_holder::type) != origin->m_mandatory_card->effects.end()
            && std::ranges::find(effects, effect_type::banglimit, &effect_holder::type) != effects.end())
        {
            throw game_error("ERROR_MANDATORY_CARD", origin->m_mandatory_card);
        }

        int diff = targets.size() - effects.size();
        if (card_ptr->optionals.last_is(effect_type::repeatable)) {
            if (diff < 0 || diff % card_ptr->optionals.size() != 0
                || (card_ptr->optionals.back().effect_value > 0
                    && diff > (card_ptr->optionals.size() * card_ptr->optionals.back().effect_value)))
            {
                throw game_error("ERROR_INVALID_TARGETS");
            }
        } else {
            if (diff != 0 && diff != card_ptr->optionals.size()) throw game_error("ERROR_INVALID_TARGETS");
        }

        mth_target_list target_list;
        std::ranges::for_each(targets, [&, it = effects.begin(), end = effects.end()] (const play_card_target &target) mutable {
            const effect_holder &e = *it++;
            if (it == end) {
                it = card_ptr->optionals.begin();
                end = card_ptr->optionals.end();
            }
            std::visit(util::overloaded{
                [&](target_none) {
                    if (e.target != play_card_target_type::none) throw game_error("ERROR_INVALID_ACTION");
                    if (e.is(effect_type::mth_add)) target_list.emplace_back(nullptr, nullptr);

                    e.verify(card_ptr, origin);
                },
                [&](target_player args) {
                    if (e.target != play_card_target_type::player) throw game_error("ERROR_INVALID_ACTION");

                    verify_effect_player_target(e.player_filter, args.target);
                    e.verify(card_ptr, origin, args.target);

                    if (e.is(effect_type::mth_add)) target_list.emplace_back(args.target, nullptr);
                },
                [&](target_card args) {
                    if (e.target != play_card_target_type::card) throw game_error("ERROR_INVALID_ACTION");

                    verify_effect_card_target(e, args.target, args.target_card);
                    e.verify(card_ptr, origin, args.target, args.target_card);

                    if (e.is(effect_type::mth_add)) target_list.emplace_back(args.target, args.target_card);
                },
                [&](target_other_players) {
                    if (e.target != play_card_target_type::other_players) throw game_error("ERROR_INVALID_ACTION");
                    for (auto *p = origin;;) {
                        p = origin->m_game->get_next_player(p);
                        if (p == origin) break;
                        e.verify(card_ptr, origin, p);
                    }
                },
                [&](target_cards_other_players const& args) {
                    if (e.target != play_card_target_type::cards_other_players) throw game_error("ERROR_INVALID_ACTION");

                    if (!std::ranges::all_of(origin->m_game->m_players | std::views::filter(&player::alive), [&](const player &p) {
                        int found = std::ranges::count(args.target_cards, &p, &card::owner);
                        if (p.m_hand.empty() && p.m_table.empty()) return found == 0;
                        if (&p == origin) return found == 0;
                        else return found == 1;
                    })) throw game_error("ERROR_INVALID_TARGETS");
                    std::ranges::for_each(args.target_cards, [&](card *c) {
                        if (!c->owner) throw game_error("ERROR_INVALID_ACTION");
                        e.verify(card_ptr, origin, c->owner, c);
                    });
                }
            }, target);
        });

        card_ptr->multi_target_handler.verify(card_ptr, origin, target_list);
    }

    void play_card_verify::log_played_card() const {
        origin->m_game->send_card_update(card_ptr);
        switch (card_ptr->pocket) {
        case pocket_type::player_hand:
        case pocket_type::scenario_card:
            origin->m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CARD" : "LOG_PLAYED_CARD", card_ptr, origin);
            break;
        case pocket_type::player_table:
            origin->m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CARD" : "LOG_PLAYED_TABLE_CARD", card_ptr, origin);
            break;
        case pocket_type::player_character:
            origin->m_game->add_log(is_response ?
                card_ptr->responses.first_is(effect_type::drawing)
                    ? "LOG_DRAWN_WITH_CHARACTER"
                    : "LOG_RESPONDED_WITH_CHARACTER"
                : "LOG_PLAYED_CHARACTER", card_ptr, origin);
            break;
        case pocket_type::shop_selection:
            origin->m_game->add_log("LOG_BOUGHT_CARD", card_ptr, origin);
            break;
        }
    }

    opt_fmt_str play_card_verify::check_prompt() {
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        
        auto effect_it = effects.begin();
        auto effect_end = effects.end();

        mth_target_list target_list;

        for (const auto &t : targets) {
            auto prompt_message = std::visit(util::overloaded{
                [&](target_none) -> opt_fmt_str {
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(nullptr, nullptr);
                        return std::nullopt;
                    } else {
                        return effect_it->on_prompt(card_ptr, origin);
                    }
                },
                [&](target_player args) -> opt_fmt_str {
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(args.target, nullptr);
                        return std::nullopt;
                    } else {
                        return effect_it->on_prompt(card_ptr, origin, args.target);
                    }
                },
                [&](target_other_players) -> opt_fmt_str {
                    opt_fmt_str msg = std::nullopt;
                    for (auto *p = origin;;) {
                        p = origin->m_game->get_next_player(p);
                        if (p == origin) {
                            return msg;
                        } else if (!(msg = effect_it->on_prompt(card_ptr, origin, p))) {
                            return std::nullopt;
                        }
                    }
                },
                [&](target_card args) -> opt_fmt_str {
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(args.target, args.target_card);
                        return std::nullopt;
                    } else {
                        return effect_it->on_prompt(card_ptr, origin, args.target, args.target_card);
                    }
                },
                [&](target_cards_other_players const& args) -> opt_fmt_str {
                    opt_fmt_str msg = std::nullopt;
                    for (card *target_card : args.target_cards) {
                        if (!(msg = effect_it->on_prompt(card_ptr, origin, target_card->owner, target_card))) {
                            return std::nullopt;
                        }
                    }
                    return msg;
                }
            }, t);
            if (prompt_message) {
                return prompt_message;
            }
            if (++effect_it == effect_end) {
                effect_it = card_ptr->optionals.begin();
                effect_end = card_ptr->optionals.end();
            }
        }

        return card_ptr->multi_target_handler.on_prompt(card_ptr, origin, target_list);
    }

    opt_fmt_str play_card_verify::check_prompt_equip(player *target) {
        for (const auto &e : card_ptr->equips) {
            if (auto prompt_message = e.on_prompt(card_ptr, target)) {
                return prompt_message;
            }
        }

        return std::nullopt;
    }

    void play_card_verify::do_play_card() const {
        if (origin->m_mandatory_card == card_ptr) {
            origin->m_mandatory_card = nullptr;
        }
        
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        log_played_card();
        if (std::ranges::find(effects, effect_type::play_card_action, &effect_holder::type) == effects.end()) {
            origin->play_card_action(card_ptr);
        }
        
        auto effect_it = effects.begin();
        auto effect_end = effects.end();

        mth_target_list target_list;
        for (const auto &t : targets) {
            std::visit(util::overloaded{
                [&](target_none) {
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(nullptr, nullptr);
                    } else {
                        effect_it->on_play(card_ptr, origin, effect_flags{});
                    }
                },
                [&](target_player args) {
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(args.target, nullptr);
                    } else {
                        if (args.target != origin && args.target->immune_to(origin->chosen_card_or(card_ptr))) {
                            if (effect_it->is(effect_type::bangcard)) {
                                request_bang req{card_ptr, origin, args.target};
                                origin->m_game->call_event<event_type::apply_bang_modifier>(origin, &req);
                            }
                        } else {
                            auto flags = effect_flags::single_target;
                            if (card_ptr->sign && card_ptr->color == card_color_type::brown && !effect_it->is(effect_type::bangcard)) {
                                flags |= effect_flags::escapable;
                            }
                            effect_it->on_play(card_ptr, origin, args.target, flags);
                        }
                    }
                },
                [&](target_other_players) {
                    std::vector<player *> targets;
                    for (auto *p = origin;;) {
                        p = origin->m_game->get_next_player(p);
                        if (p == origin) break;
                        targets.push_back(p);
                    }
                    
                    effect_flags flags{};
                    if (card_ptr->sign && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (targets.size() == 1) {
                        flags |= effect_flags::single_target;
                    }
                    for (auto *p : targets) {
                        if (!p->immune_to(origin->chosen_card_or(card_ptr))) {
                            effect_it->on_play(card_ptr, origin, p, flags);
                        }
                    }
                },
                [&](target_card args) {
                    auto flags = effect_flags::single_target;
                    if (card_ptr->sign && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(args.target, args.target_card);
                    } else if (args.target == origin) {
                        effect_it->on_play(card_ptr, origin, args.target, args.target_card, flags);
                    } else if (!args.target->immune_to(origin->chosen_card_or(card_ptr))) {
                        if (args.target_card->pocket == pocket_type::player_hand) {
                            effect_it->on_play(card_ptr, origin, args.target, args.target->random_hand_card(), flags);
                        } else {
                            effect_it->on_play(card_ptr, origin, args.target, args.target_card, flags);
                        }
                    }
                },
                [&](target_cards_other_players const& args) {
                    effect_flags flags{};
                    if (card_ptr->sign && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (args.target_cards.size() == 1) {
                        flags |= effect_flags::single_target;
                    }
                    for (card *target_card : args.target_cards) {
                        if (target_card->pocket == pocket_type::player_hand) {
                            effect_it->on_play(card_ptr, origin, target_card->owner, target_card->owner->random_hand_card(), flags);
                        } else {
                            effect_it->on_play(card_ptr, origin, target_card->owner, target_card, flags);
                        }
                    }
                }
            }, t);
            if (++effect_it == effect_end) {
                effect_it = card_ptr->optionals.begin();
                effect_end = card_ptr->optionals.end();
            }
        }

        card_ptr->multi_target_handler.on_play(card_ptr, origin, target_list);
        origin->m_game->call_event<event_type::on_effect_end>(origin, card_ptr);
    }

}