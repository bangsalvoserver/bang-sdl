#include "play_verify.h"

#include "player.h"
#include "game.h"

#include "effects/base/requests.h"

namespace banggame {
    using namespace enums::flag_operators;

    std::optional<game_error> check_player_filter(player *origin, target_player_filter filter, player *target) {
        if (target->alive() == bool(filter & target_player_filter::dead)) {
            return game_error("ERROR_TARGET_DEAD");
        }

        for (auto value : enums::enum_flag_values(filter)) {
            switch (value) {
            case target_player_filter::self:
                if (target != origin) {
                    return game_error("ERROR_TARGET_NOT_SELF");
                }
                break;
            case target_player_filter::notself:
                if (target == origin) {
                    return game_error("ERROR_TARGET_SELF");
                }
                break;
            case target_player_filter::notsheriff:
                if (target->m_role == player_role::sheriff) {
                    return game_error("ERROR_TARGET_SHERIFF");
                }
                break;
            case target_player_filter::reachable:
                if (!origin->m_weapon_range || origin->m_game->calc_distance(origin, target) > origin->m_weapon_range + origin->m_range_mod) {
                    return game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::range_1:
                if (origin->m_game->calc_distance(origin, target) > 1 + origin->m_range_mod) {
                    return game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::range_2:
                if (origin->m_game->calc_distance(origin, target) > 2 + origin->m_range_mod) {
                    return game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::dead:
                break;
            default: return game_error("ERROR_INVALID_ACTION");
            }
        }
        return std::nullopt;
    }

    std::optional<game_error> check_card_filter(player *origin, target_card_filter filter, card *target) {
        if ((target->color == card_color_type::black) != bool(filter & target_card_filter::black)) {
            return game_error("ERROR_TARGET_BLACK_CARD");
        }

        for (auto value : enums::enum_flag_values(filter)) {
            switch (value) {
            case target_card_filter::table:
                if (target->pocket != pocket_type::player_table) {
                    return game_error("ERROR_TARGET_NOT_TABLE_CARD");
                }
                break;
            case target_card_filter::hand:
                if (target->pocket != pocket_type::player_hand) {
                    return game_error("ERROR_TARGET_NOT_HAND_CARD");
                }
                break;
            case target_card_filter::blue:
                if (target->color != card_color_type::blue) {
                    return game_error("ERROR_TARGET_NOT_BLUE_CARD");
                }
                break;
            case target_card_filter::clubs:
                if (origin->get_card_sign(target).suit != card_suit::clubs) {
                    return game_error("ERROR_TARGET_NOT_CLUBS");
                }
                break;
            case target_card_filter::bang:
                if (!origin->is_bangcard(target) || !target->equips.empty()) {
                    return game_error("ERROR_TARGET_NOT_BANG");
                }
                break;
            case target_card_filter::missed:
                if (!target->responses.last_is(effect_type::missedcard)) {
                    return game_error("ERROR_TARGET_NOT_MISSED");
                }
                break;
            case target_card_filter::beer:
                if (!target->effects.first_is(effect_type::beer)) {
                    return game_error("ERROR_TARGET_NOT_BEER");
                }
                break;
            case target_card_filter::bronco:
                if (!target->equips.last_is(equip_type::bronco)) {
                    return game_error("ERROR_TARGET_NOT_BRONCO");
                }
                break;
            case target_card_filter::cube_slot:
            case target_card_filter::cube_slot_card:
                if (target != target->owner->m_characters.front() && target->color != card_color_type::orange)
                    return game_error("ERROR_TARGET_NOT_CUBE_SLOT");
                break;
            case target_card_filter::can_repeat:
            case target_card_filter::black:
                break;
            default: return game_error("ERROR_INVALID_ACTION");
            }
        }
        return std::nullopt;
    }

    template<typename T>
    void maybe_throw(std::optional<T> &&e) {
        if (e) {
            throw std::move(*e);
        }
    }

    void play_card_verify::verify_modifiers() {
        for (card *mod_card : modifiers) {
            if (origin->m_game->is_disabled(mod_card)) {
                throw game_error("ERROR_CARD_IS_DISABLED", mod_card);
            }
            if (mod_card->modifier == card_modifier_type::bangmod && !card_ptr->effects.last_is(effect_type::bangcard)) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            for (const auto &e : mod_card->effects) {
                e.verify(mod_card, origin);
            }
        }
    }

    void play_card_verify::play_modifiers() const {
        for (card *mod_card : modifiers) {
            origin->log_played_card(mod_card, false);
            origin->play_card_action(mod_card);
            for (effect_holder &e : mod_card->effects) {
                e.on_play(mod_card, origin, effect_flags{});
            }
        }
    }

    player *play_card_verify::verify_equip_target() {
        if (origin->m_game->is_disabled(card_ptr)) {
            throw game_error("ERROR_CARD_IS_DISABLED", card_ptr);
        }
        if (origin->m_game->has_scenario(scenario_flags::judge)) {
            throw game_error("ERROR_CANT_EQUIP_CARDS");
        }
        player *target = origin;
        if (!card_ptr->equips.empty()) {
            if (targets.size() != 1 || !std::holds_alternative<target_player_t>(targets.front())) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            target = std::get<target_player_t>(targets.front()).target;
            maybe_throw(check_player_filter(origin, card_ptr->equips.front().player_filter, target));
        }
        if (auto *card = target->find_equipped_card(card_ptr)) {
            throw game_error("ERROR_DUPLICATED_CARD", card);
        }
        if (card_ptr->color == card_color_type::orange && origin->m_game->m_cubes.size() < 3) {
            throw game_error("ERROR_NOT_ENOUGH_CUBES");
        }
        return target;
    }

    template<typename T, T Diff = 1>
    struct raii_modifier {
        T *ptr = nullptr;

        void set(T &value) {
            ptr = &value;
            *ptr += Diff;
        }
        ~raii_modifier() {
            if (ptr) {
                *ptr -= Diff;
            }
        }
    };

    void play_card_verify::verify_card_targets() {
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;

        if (origin->m_game->is_disabled(card_ptr)) {
            throw game_error("ERROR_CARD_IS_DISABLED", card_ptr);
        }
        if (card_ptr->inactive) {
            throw game_error("ERROR_CARD_INACTIVE", card_ptr);
        }

        if (origin->m_mandatory_card && origin->m_mandatory_card != card_ptr
            && std::ranges::find(origin->m_mandatory_card->effects, effect_type::banglimit, &effect_holder::type) != origin->m_mandatory_card->effects.end()
            && std::ranges::find(effects, effect_type::banglimit, &effect_holder::type) != effects.end())
        {
            throw game_error("ERROR_MANDATORY_CARD", origin->m_mandatory_card);
        }

        if (origin->m_forced_card
            && card_ptr != origin->m_forced_card
            && std::ranges::find(modifiers, origin->m_forced_card) == modifiers.end()) {
            throw game_error("ERROR_INVALID_ACTION");
        }

        raii_modifier<decltype(player::m_range_mod), 50> _belltower_mod;
        raii_modifier<decltype(player::m_bangs_per_turn)> _bandolier_mod;
        raii_modifier<decltype(player::m_bangs_per_turn), 10> _leevankliff_mod;

        for (card *c : modifiers) {
            switch (c->modifier) {
            case card_modifier_type::belltower: _belltower_mod.set(origin->m_range_mod); break;
            case card_modifier_type::bandolier: _bandolier_mod.set(origin->m_bangs_per_turn); break;
            case card_modifier_type::leevankliff: _leevankliff_mod.set(origin->m_bangs_per_turn); break;
            }
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
        
        auto effect_it = effects.begin();
        auto effect_end = effects.end();

        target_list mth_targets;
        for (const auto &t : targets) {
            const auto &e = *effect_it;
            if (++effect_it == effect_end) {
                effect_it = card_ptr->optionals.begin();
                effect_end = card_ptr->optionals.end();
            }

            if (e.is(effect_type::mth_add)) {
                mth_targets.push_back(t);
            }

            std::visit(util::overloaded{
                [this, &e](target_none_t args) {
                    if (e.target != play_card_target_type::none) {
                        throw game_error("ERROR_INVALID_ACTION");
                    } else {
                        e.verify(card_ptr, origin);
                    }
                },
                [this, &e](target_player_t args) {
                    if (e.target != play_card_target_type::player) {
                        throw game_error("ERROR_INVALID_ACTION");
                    } else {
                        maybe_throw(check_player_filter(origin, e.player_filter, args.target));
                        e.verify(card_ptr, origin, args.target);
                    }
                },
                [this, &e](target_card_t args) {
                    if (e.target != play_card_target_type::card) {
                        throw game_error("ERROR_INVALID_ACTION");
                    } else if (!args.target->owner) {
                        throw game_error("ERROR_INVALID_ACTION");
                    } else {
                        maybe_throw(check_player_filter(origin, e.player_filter, args.target->owner));
                        maybe_throw(check_card_filter(origin, e.card_filter, args.target));
                        e.verify(card_ptr, origin, args.target);
                    }
                },
                [this, &e](target_other_players_t args) {
                    if (e.target != play_card_target_type::other_players) {
                        throw game_error("ERROR_INVALID_ACTION");
                    } else {
                        for (auto *p = origin;;) {
                            p = origin->m_game->get_next_player(p);
                            if (p == origin) break;
                            e.verify(card_ptr, origin, p);
                        }
                    }
                },
                [this, &e](target_cards_other_players_t const& args) {
                    if (e.target != play_card_target_type::cards_other_players) {
                        throw game_error("ERROR_INVALID_ACTION");
                    } else if (!std::ranges::all_of(origin->m_game->m_players | std::views::filter(&player::alive), [&](const player &p) {
                        int found = std::ranges::count(args.target_cards, &p, &card::owner);
                        if (p.m_hand.empty() && p.m_table.empty()) return found == 0;
                        if (&p == origin) return found == 0;
                        else return found == 1;
                    })) {
                        throw game_error("ERROR_INVALID_TARGETS");
                    } else {
                        for (card *c : args.target_cards) {
                            e.verify(card_ptr, origin, c);
                        }
                    }
                }
            }, t);
        }

        card_ptr->multi_target_handler.verify(card_ptr, origin, mth_targets);
    }

    opt_fmt_str play_card_verify::check_prompt() {
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        
        auto effect_it = effects.begin();
        auto effect_end = effects.end();

        target_list mth_targets;
        for (const auto &t : targets) {
            const auto &e = *effect_it;
            if (++effect_it == effect_end) {
                effect_it = card_ptr->optionals.begin();
                effect_end = card_ptr->optionals.end();
            }

            if (e.is(effect_type::mth_add)) {
                mth_targets.push_back(t);
            }

            if (auto prompt_message = std::visit(util::overloaded{
                [this, &e](target_none_t args) -> opt_fmt_str {
                    return e.on_prompt(card_ptr, origin);
                },
                [this, &e](target_player_t args) -> opt_fmt_str {
                    return e.on_prompt(card_ptr, origin, args.target);
                },
                [this, &e](target_other_players_t args) -> opt_fmt_str {
                    opt_fmt_str msg = std::nullopt;
                    for (auto *p = origin;;) {
                        p = origin->m_game->get_next_player(p);
                        if (p == origin || !(msg = e.on_prompt(card_ptr, origin, p))) {
                            break;
                        }
                    }
                    return msg;
                },
                [this, &e](target_card_t args) -> opt_fmt_str {
                    return e.on_prompt(card_ptr, origin, args.target);
                },
                [this, &e](target_cards_other_players_t const& args) -> opt_fmt_str {
                    opt_fmt_str msg = std::nullopt;
                    for (card *target_card : args.target_cards) {
                        if (!(msg = e.on_prompt(card_ptr, origin, target_card))) {
                            break;
                        }
                    }
                    return msg;
                }
            }, t)) {
                return prompt_message;
            }
        }

        return card_ptr->multi_target_handler.on_prompt(card_ptr, origin, mth_targets);
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
        origin->m_forced_card = nullptr;
        
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        origin->log_played_card(card_ptr, is_response);
        if (std::ranges::find(effects, effect_type::play_card_action, &effect_holder::type) == effects.end()) {
            origin->play_card_action(card_ptr);
        }
        
        auto effect_it = effects.begin();
        auto effect_end = effects.end();

        target_list mth_targets;
        for (const auto &t : targets) {
            auto &e = *effect_it;
            if (++effect_it == effect_end) {
                effect_it = card_ptr->optionals.begin();
                effect_end = card_ptr->optionals.end();
            }

            if (e.is(effect_type::mth_add)) {
                mth_targets.push_back(t);
            }

            std::visit(util::overloaded{
                [this, &e](target_none_t args) {
                    e.on_play(card_ptr, origin, effect_flags{});
                },
                [this, &e](target_player_t args) {
                    if (args.target != origin && args.target->immune_to(origin->chosen_card_or(card_ptr))) {
                        if (e.is(effect_type::bangcard)) {
                            request_bang req{card_ptr, origin, args.target};
                            origin->m_game->call_event<event_type::apply_bang_modifier>(origin, &req);
                        }
                    } else {
                        auto flags = effect_flags::single_target;
                        if (card_ptr->sign && card_ptr->color == card_color_type::brown && !e.is(effect_type::bangcard)) {
                            flags |= effect_flags::escapable;
                        }
                        e.on_play(card_ptr, origin, args.target, flags);
                    }
                },
                [this, &e](target_other_players_t args) {
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
                            e.on_play(card_ptr, origin, p, flags);
                        }
                    }
                },
                [this, &e](target_card_t args) {
                    auto flags = effect_flags::single_target;
                    if (card_ptr->sign && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (args.target->owner == origin) {
                        e.on_play(card_ptr, origin, args.target, flags);
                    } else if (!args.target->owner->immune_to(origin->chosen_card_or(card_ptr))) {
                        if (args.target->pocket == pocket_type::player_hand) {
                            e.on_play(card_ptr, origin, args.target->owner->random_hand_card(), flags);
                        } else {
                            e.on_play(card_ptr, origin, args.target, flags);
                        }
                    }
                },
                [this, &e](target_cards_other_players_t const& args) {
                    effect_flags flags{};
                    if (card_ptr->sign && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (args.target_cards.size() == 1) {
                        flags |= effect_flags::single_target;
                    }
                    for (card *target_card : args.target_cards) {
                        if (target_card->pocket == pocket_type::player_hand) {
                            e.on_play(card_ptr, origin, target_card->owner->random_hand_card(), flags);
                        } else {
                            e.on_play(card_ptr, origin, target_card, flags);
                        }
                    }
                }
            }, t);
        }

        card_ptr->multi_target_handler.on_play(card_ptr, origin, mth_targets);
        origin->m_game->call_event<event_type::on_effect_end>(origin, card_ptr);
    }

    void play_card_verify::verify_and_play() {
        switch(card_ptr->pocket) {
        case pocket_type::player_hand:
            if (!modifiers.empty() && modifiers.front()->modifier == card_modifier_type::leevankliff) {
                card *bang_card = std::exchange(card_ptr, origin->m_last_played_card);
                verify_modifiers();
                verify_card_targets();
                origin->prompt_then(check_prompt(), [*this, bang_card]{
                    origin->m_game->move_card(bang_card, pocket_type::discard_pile);
                    origin->m_game->call_event<event_type::on_play_hand_card>(origin, bang_card);
                    do_play_card();
                    origin->set_last_played_card(nullptr);
                });
            } else if (card_ptr->color == card_color_type::brown) {
                verify_modifiers();
                verify_card_targets();
                origin->prompt_then(check_prompt(), [*this]{
                    play_modifiers();
                    do_play_card();
                    origin->set_last_played_card(card_ptr);
                });
            } else {
                player *target = verify_equip_target();
                origin->prompt_then(check_prompt_equip(target), [*this, target]{
                    if (origin->m_mandatory_card == card_ptr) {
                        origin->m_mandatory_card = nullptr;
                    }
                    origin->m_forced_card = nullptr;
                    target->equip_card(card_ptr);
                    if (origin == target) {
                        origin->m_game->add_log("LOG_EQUIPPED_CARD", card_ptr, origin);
                    } else {
                        origin->m_game->add_log("LOG_EQUIPPED_CARD_TO", card_ptr, origin, target);
                    }
                    switch (card_ptr->color) {
                    case card_color_type::blue:
                        if (origin->m_game->has_expansion(card_expansion_type::armedanddangerous)) {
                            origin->queue_request_add_cube(card_ptr);
                        }
                        break;
                    case card_color_type::green:
                        card_ptr->inactive = true;
                        origin->m_game->add_public_update<game_update_type::tap_card>(card_ptr->id, true);
                        break;
                    case card_color_type::orange:
                        origin->add_cubes(card_ptr, 3);
                        break;
                    }
                    origin->m_game->call_event<event_type::on_equip>(origin, target, card_ptr);
                    origin->set_last_played_card(nullptr);
                    origin->m_game->call_event<event_type::on_effect_end>(origin, card_ptr);
                });
            }
            break;
        case pocket_type::player_character:
        case pocket_type::player_table:
        case pocket_type::scenario_card:
        case pocket_type::specials:
            verify_modifiers();
            verify_card_targets();
            origin->prompt_then(check_prompt(), [*this]{
                play_modifiers();
                do_play_card();
                origin->set_last_played_card(nullptr);
            });
            break;
        case pocket_type::hidden_deck:
            if (std::ranges::find(modifiers, card_modifier_type::shopchoice, &card::modifier) == modifiers.end()) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            [[fallthrough]];
        case pocket_type::shop_selection: {
            int cost = card_ptr->buy_cost();
            verify_modifiers();
            for (card *c : modifiers) {
                switch (c->modifier) {
                case card_modifier_type::discount:
                    --cost;
                    break;
                case card_modifier_type::shopchoice:
                    if (!c->effects.first_is(card_ptr->effects.front().type)) {
                        throw game_error("ERROR_INVALID_ACTION");
                    }
                    cost += c->buy_cost();
                    break;
                }
            }
            if (origin->m_game->m_shop_selection.size() > 3) {
                cost = 0;
            }
            if (origin->m_gold < cost) {
                throw game_error("ERROR_NOT_ENOUGH_GOLD");
            }
            if (card_ptr->color == card_color_type::brown) {
                verify_card_targets();
                origin->prompt_then(check_prompt(), [*this, cost]{
                    play_modifiers();
                    origin->add_gold(-cost);
                    do_play_card();
                    origin->set_last_played_card(nullptr);
                    origin->m_game->queue_action([m_game = origin->m_game]{
                        while (m_game->m_shop_selection.size() < 3) {
                            m_game->draw_shop_card();
                        }
                    });
                });
            } else {
                player *target = verify_equip_target();
                origin->prompt_then(check_prompt_equip(target), [*this, target, cost]{
                    origin->m_forced_card = nullptr;
                    play_modifiers();
                    origin->add_gold(-cost);
                    target->equip_card(card_ptr);
                    origin->set_last_played_card(nullptr);
                    if (origin == target) {
                        origin->m_game->add_log("LOG_BOUGHT_EQUIP", card_ptr, origin);
                    } else {
                        origin->m_game->add_log("LOG_BOUGHT_EQUIP_TO", card_ptr, origin, target);
                    }
                    origin->m_game->queue_action([m_game = origin->m_game]{
                        while (m_game->m_shop_selection.size() < 3) {
                            m_game->draw_shop_card();
                        }
                    });
                });
            }
            break;
        }
        default:
            throw game_error("play_card: invalid card"_nonloc);
        }
    }

}