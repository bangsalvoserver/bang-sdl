#include "player.h"

#include "game.h"

#include "holders.h"
#include "server/net_enums.h"

#include "effects/base/requests.h"
#include "effects/armedanddangerous/requests.h"
#include "effects/valleyofshadows/requests.h"

#include <cassert>

namespace banggame {
    using namespace enums::flag_operators;

    void player::equip_card(card *target) {
        for (auto &e : target->equips) {
            e.on_pre_equip(target, this);
        }

        m_game->move_to(target, pocket_type::player_table, true, this, show_card_flags::show_everyone);
        equip_if_enabled(target);
        target->usages = 0;
    }

    void player::equip_if_enabled(card *target_card) {
        if (!m_game->is_disabled(target_card)) {
            for (auto &e : target_card->equips) {
                e.on_equip(target_card, this);
            }
        }
    }

    void player::unequip_if_enabled(card *target_card) {
        if (!m_game->is_disabled(target_card)) {
            for (auto &e : target_card->equips) {
                e.on_unequip(target_card, this);
            }
        }
    }

    int player::get_initial_cards() {
        int value = m_max_hp;
        m_game->call_event<event_type::apply_initial_cards_modifier>(this, value);
        return value;
    }

    int player::max_cards_end_of_turn() {
        int n = m_hp;
        m_game->call_event<event_type::apply_maxcards_modifier>(this, n);
        return n;
    }

    bool player::alive() const {
        return !check_player_flags(player_flags::dead) || check_player_flags(player_flags::ghost)
            || (m_game->m_playing == this && m_game->has_scenario(scenario_flags::ghosttown));
    }

    card *player::find_equipped_card(card *card) {
        auto it = std::ranges::find(m_table, card->name, &card::name);
        if (it != m_table.end()) {
            return *it;
        } else {
            return nullptr;
        }
    }

    card *player::random_hand_card() {
        return m_hand[std::uniform_int_distribution<int>(0, m_hand.size() - 1)(m_game->rng)];
    }

    card *player::chosen_card_or(card *c) {
        m_game->call_event<event_type::apply_chosen_card_modifier>(this, c);
        return c;
    }

    std::vector<card *>::iterator player::move_card_to(card *target_card, pocket_type pocket, bool known, player *owner, show_card_flags flags) {
        if (target_card->owner != this) throw game_error("ERROR_PLAYER_DOES_NOT_OWN_CARD");
        if (target_card->pocket == pocket_type::player_table) {
            if (target_card->inactive) {
                target_card->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(target_card->id, false);
            }
            drop_all_cubes(target_card);
            auto it = m_game->move_to(target_card, pocket, known, owner, flags);
            m_game->call_event<event_type::post_discard_card>(this, target_card);
            unequip_if_enabled(target_card);
            return it;
        } else if (target_card->pocket == pocket_type::player_hand) {
            return m_game->move_to(target_card, pocket, known, owner, flags);
        } else {
            throw game_error("ERROR_CARD_NOT_FOUND");
        }
    }

    void player::discard_card(card *target) {
        move_card_to(target, target->color == card_color_type::black
            ? pocket_type::shop_discard
            : pocket_type::discard_pile, true);
    }

    void player::steal_card(player *target, card *target_card) {
        target->move_card_to(target_card, pocket_type::player_hand, true, this);
    }

    void player::damage(card *origin_card, player *origin, int value, bool is_bang, bool instant) {
        if (!check_player_flags(player_flags::ghost) && !(m_hp == 0 && m_game->has_scenario(scenario_flags::ghosttown))) {
            if (instant || !m_game->has_expansion(card_expansion_type::valleyofshadows | card_expansion_type::canyondiablo)) {
                m_hp -= value;
                m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
                m_game->add_log(value == 1 ? "LOG_TAKEN_DAMAGE" : "LOG_TAKEN_DAMAGE_PLURAL", origin_card, this, value);
                if (m_hp <= 0) {
                    m_game->queue_request_front<request_death>(origin_card, origin, this);
                }
                if (m_game->has_expansion(card_expansion_type::goldrush)) {
                    if (origin && origin->m_game->m_playing == origin && origin != this) {
                        origin->add_gold(value);
                    }
                }
                m_game->call_event<event_type::on_hit>(origin_card, origin, this, value, is_bang);
            } else {
                m_game->queue_request_front<timer_damaging>(origin_card, origin, this, value, is_bang);
            }
        }
    }

    void player::heal(int value) {
        if (!check_player_flags(player_flags::ghost) && !(m_hp == 0 && m_game->has_scenario(scenario_flags::ghosttown)) && m_hp != m_max_hp) {
            m_hp = std::min<int8_t>(m_hp + value, m_max_hp);
            m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
            if (value == 1) {
                m_game->add_log("LOG_HEALED", this);
            } else {
                m_game->add_log("LOG_HEALED_PLURAL", this, value);
            }
        }
    }

    void player::add_gold(int amount) {
        if (amount) {
            m_gold += amount;
            m_game->add_public_update<game_update_type::player_gold>(id, m_gold);
        }
    }

    bool player::immune_to(card *c) {
        bool value = false;
        m_game->call_event<event_type::apply_immunity_modifier>(c, this, value);
        return value;
    }

    bool player::can_respond_with(card *c) {
        return !m_game->is_disabled(c) && !c->responses.empty()
            && std::ranges::all_of(c->responses, [&](const effect_holder &e) {
                return e.can_respond(c, this);
            });
    }

    bool player::can_receive_cubes() const {
        if (m_game->m_cubes.empty()) return false;
        if (m_characters.front()->cubes.size() < 4) return true;
        return std::ranges::any_of(m_table, [](const card *card) {
            return card->color == card_color_type::orange && card->cubes.size() < 4;
        });
    }

    bool player::can_escape(player *origin, card *origin_card, effect_flags flags) const {
        if (bool(flags & effect_flags::escapable)
            && m_game->has_expansion(card_expansion_type::valleyofshadows)) return true;
        
        bool value = false;
        m_game->call_event<event_type::apply_escapable_modifier>(origin_card, origin, this, flags, value);
        return value;
    }
    
    void player::add_cubes(card *target, int ncubes) {
        for (;ncubes!=0 && !m_game->m_cubes.empty() && target->cubes.size() < 4; --ncubes) {
            int cube = m_game->m_cubes.back();
            m_game->m_cubes.pop_back();

            target->cubes.push_back(cube);
            m_game->add_public_update<game_update_type::move_cube>(cube, target->id);
        }
    }

    static void check_orange_card_empty(player *owner, card *target) {
        if (target->cubes.empty() && target->pocket != pocket_type::player_character && target->pocket != pocket_type::player_backup) {
            owner->m_game->move_to(target, pocket_type::discard_pile);
            owner->m_game->call_event<event_type::post_discard_orange_card>(owner, target);
            owner->unequip_if_enabled(target);
        }
    }

    void player::pay_cubes(card *target, int ncubes) {
        for (;ncubes!=0 && !target->cubes.empty(); --ncubes) {
            int cube = target->cubes.back();
            target->cubes.pop_back();

            m_game->m_cubes.push_back(cube);
            m_game->add_public_update<game_update_type::move_cube>(cube, 0);
        }
        check_orange_card_empty(this, target);
    }

    void player::move_cubes(card *origin, card *target, int ncubes) {
        for(;ncubes!=0 && !origin->cubes.empty(); --ncubes) {
            int cube = origin->cubes.back();
            origin->cubes.pop_back();
            
            if (target->cubes.size() < 4) {
                target->cubes.push_back(cube);
                m_game->add_public_update<game_update_type::move_cube>(cube, target->id);
            } else {
                m_game->m_cubes.push_back(cube);
                m_game->add_public_update<game_update_type::move_cube>(cube, 0);
            }
        }
        check_orange_card_empty(this, origin);
    }

    void player::drop_all_cubes(card *target) {
        for (int id : target->cubes) {
            m_game->m_cubes.push_back(id);
            m_game->add_public_update<game_update_type::move_cube>(id, 0);
        }
        target->cubes.clear();
    }

    void player::add_to_hand(card *target) {
        m_game->move_to(target, pocket_type::player_hand, true, this);
    }

    void player::set_last_played_card(card *c) {
        m_last_played_card = c;
        m_game->add_private_update<game_update_type::last_played_card>(this, c ? c->id : 0);
        remove_player_flags(player_flags::start_of_turn);
    }

    void player::set_forced_card(card *c) {
        m_forced_card = c;
        m_game->add_private_update<game_update_type::force_play_card>(this, c ? c->id : 0);
    }

    void player::set_mandatory_card(card *c) {
        m_mandatory_card = c;
    }

    void player::verify_modifiers(card *c, const std::vector<card *> &modifiers) {
        for (card *mod_card : modifiers) {
            if (m_game->is_disabled(mod_card)) {
                throw game_error("ERROR_CARD_IS_DISABLED", mod_card);
            }
            if (mod_card->modifier != card_modifier_type::bangcard
                || std::ranges::find(c->effects, effect_type::bangcard, &effect_holder::type) == c->effects.end()) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            for (const auto &e : mod_card->effects) {
                e.verify(mod_card, this);
            }
        }
    }

    void player::play_modifiers(const std::vector<card *> &modifiers) {
        for (card *mod_card : modifiers) {
            do_play_card(mod_card, false, std::vector{mod_card->effects.size(),
                play_card_target{enums::enum_tag<play_card_target_type::none>}});
        }
    }

    void player::verify_equip_target(card *c, const std::vector<play_card_target> &targets) {
        if (m_game->is_disabled(c)) throw game_error("ERROR_CARD_IS_DISABLED", c);
        if (c->equips.empty()) return;
        if (targets.size() != 1) throw game_error("ERROR_INVALID_ACTION");
        if (targets.front().enum_index() != play_card_target_type::player) throw game_error("ERROR_INVALID_ACTION");
        int target_player_id = targets.front().get<play_card_target_type::player>();
        verify_effect_player_target(c->equips.front().player_filter, m_game->find_player(target_player_id));
    }

    bool player::is_bangcard(card *card_ptr) {
        return (check_player_flags(player_flags::treat_missed_as_bang)
                && !card_ptr->responses.empty() && card_ptr->responses.back().is(effect_type::missedcard))
            || (!card_ptr->effects.empty() && card_ptr->effects.front().is(effect_type::bangcard));
    };

    void player::verify_effect_player_target(target_player_filter filter, player *target) {
        if (target->alive() == bool(filter & target_player_filter::dead)) {
            throw game_error("ERROR_TARGET_DEAD");
        }

        std::ranges::for_each(enums::enum_flag_values(filter), [&](target_player_filter value) {
            switch (value) {
            case target_player_filter::self:
                if (target != this) {
                    throw game_error("ERROR_TARGET_NOT_SELF");
                }
                break;
            case target_player_filter::notself:
                if (target == this) {
                    throw game_error("ERROR_TARGET_SELF");
                }
                break;
            case target_player_filter::notsheriff:
                if (target->m_role == player_role::sheriff) {
                    throw game_error("ERROR_TARGET_SHERIFF");
                }
                break;
            case target_player_filter::reachable:
                if (!m_weapon_range || m_game->calc_distance(this, target) > m_weapon_range + m_range_mod) {
                    throw game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::range_1:
                if (m_game->calc_distance(this, target) > 1 + m_range_mod) {
                    throw game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::range_2:
                if (m_game->calc_distance(this, target) > 2 + m_range_mod) {
                    throw game_error("ERROR_TARGET_NOT_IN_RANGE");
                }
                break;
            case target_player_filter::dead:
                break;
            default: throw game_error("ERROR_INVALID_ACTION");
            }
        });
    }

    void player::verify_effect_card_target(const effect_holder &effect, player *target, card *target_card) {
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
                if (get_card_sign(target_card).suit != card_suit::clubs) {
                    throw game_error("ERROR_TARGET_NOT_CLUBS");
                }
                break;
            case target_card_filter::bang:
                if (!is_bangcard(target_card)) {
                    throw game_error("ERROR_TARGET_NOT_BANG");
                }
                break;
            case target_card_filter::missed:
                if (target_card->responses.empty() || !target_card->responses.back().is(effect_type::missedcard)) {
                    throw game_error("ERROR_TARGET_NOT_MISSED");
                }
                break;
            case target_card_filter::beer:
                if (target_card->effects.empty() || !target_card->effects.front().is(effect_type::beer)) {
                    throw game_error("ERROR_TARGET_NOT_BEER");
                }
                break;
            case target_card_filter::bronco:
                if (target_card->equips.empty() || !target_card->equips.back().is(equip_type::bronco)) {
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

    void player::verify_card_targets(card *card_ptr, bool is_response, const std::vector<play_card_target> &targets) {
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;

        if (m_mandatory_card && m_mandatory_card != card_ptr
            && std::ranges::find(m_mandatory_card->effects, effect_type::banglimit, &effect_holder::type) != m_mandatory_card->effects.end()
            && std::ranges::find(effects, effect_type::banglimit, &effect_holder::type) != effects.end())
        {
            throw game_error("ERROR_MANDATORY_CARD", m_mandatory_card);
        }

        int diff = targets.size() - effects.size();
        if (!card_ptr->optionals.empty() && card_ptr->optionals.back().is(effect_type::repeatable)) {
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
            enums::visit_indexed(util::overloaded{
                [&](enums::enum_tag_t<play_card_target_type::none>) {
                    if (e.target != play_card_target_type::none) throw game_error("ERROR_INVALID_ACTION");
                    if (e.is(effect_type::mth_add)) target_list.emplace_back(nullptr, nullptr);

                    e.verify(card_ptr, this);
                },
                [&](enums::enum_tag_t<play_card_target_type::player>, int target_id) {
                    if (e.target != play_card_target_type::player) throw game_error("ERROR_INVALID_ACTION");

                    player *target = m_game->find_player(target_id);

                    verify_effect_player_target(e.player_filter, target);
                    e.verify(card_ptr, this, target);

                    if (e.is(effect_type::mth_add)) target_list.emplace_back(target, nullptr);
                },
                [&](enums::enum_tag_t<play_card_target_type::card>, int target_card_id) {
                    if (e.target != play_card_target_type::card) throw game_error("ERROR_INVALID_ACTION");

                    card *target_card = m_game->find_card(target_card_id);
                    player *target = target_card->owner;
                    if (!target) throw game_error("ERROR_INVALID_ACTION");

                    verify_effect_card_target(e, target, target_card);
                    e.verify(card_ptr, this, target, target_card);

                    if (e.is(effect_type::mth_add)) target_list.emplace_back(target, target_card);
                },
                [&](enums::enum_tag_t<play_card_target_type::other_players>) {
                    if (e.target != play_card_target_type::other_players) throw game_error("ERROR_INVALID_ACTION");
                    for (auto *p = this;;) {
                        p = m_game->get_next_player(p);
                        if (p == this) break;
                        e.verify(card_ptr, this, p);
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::cards_other_players>, const std::vector<int> &target_ids) {
                    if (e.target != play_card_target_type::cards_other_players) throw game_error("ERROR_INVALID_ACTION");
                    std::vector<card *> target_cards;
                    for (int id : target_ids) {
                        target_cards.push_back(m_game->find_card(id));
                    }
                    if (!std::ranges::all_of(m_game->m_players | std::views::filter(&player::alive), [&](const player &p) {
                        int found = std::ranges::count(target_cards, &p, &card::owner);
                        if (p.m_hand.empty() && p.m_table.empty()) return found == 0;
                        if (&p == this) return found == 0;
                        else return found == 1;
                    })) throw game_error("ERROR_INVALID_TARGETS");
                    std::ranges::for_each(target_cards, [&](card *c) {
                        if (!c->owner) throw game_error("ERROR_INVALID_ACTION");
                        e.verify(card_ptr, this, c->owner, c);
                    });
                }
            }, target);
        });

        card_ptr->multi_target_handler.verify(card_ptr, this, target_list);
    }

    void player::play_card_action(card *card_ptr) {
        switch (card_ptr->pocket) {
        case pocket_type::player_hand:
            m_game->move_to(card_ptr, pocket_type::discard_pile);
            m_game->call_event<event_type::on_play_hand_card>(this, card_ptr);
            break;
        case pocket_type::player_table:
            if (card_ptr->color == card_color_type::green) {
                m_game->move_to(card_ptr, pocket_type::discard_pile);
            }
            break;
        case pocket_type::shop_selection:
            if (card_ptr->color == card_color_type::brown) {
                m_game->move_to(card_ptr, pocket_type::shop_discard);
            }
            break;
        default:
            break;
        }
    }

    void player::log_played_card(card *card_ptr, bool is_response) {
        m_game->send_card_update(*card_ptr);
        switch (card_ptr->pocket) {
        case pocket_type::player_hand:
        case pocket_type::scenario_card:
            m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CARD" : "LOG_PLAYED_CARD", card_ptr, this);
            break;
        case pocket_type::player_table:
            m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CARD" : "LOG_PLAYED_TABLE_CARD", card_ptr, this);
            break;
        case pocket_type::player_character:
            m_game->add_log(is_response ?
                card_ptr->responses.front().is(effect_type::drawing)
                    ? "LOG_DRAWN_WITH_CHARACTER"
                    : "LOG_RESPONDED_WITH_CHARACTER"
                : "LOG_PLAYED_CHARACTER", card_ptr, this);
            break;
        case pocket_type::shop_selection:
            m_game->add_log("LOG_BOUGHT_CARD", card_ptr, this);
            break;
        }
    }

    void player::check_prompt(card *card_ptr, bool is_response, const std::vector<play_card_target> &targets, std::function<void()> &&fun) {
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        
        auto effect_it = effects.begin();
        auto effect_end = effects.end();

        mth_target_list target_list;

        for (auto &t : targets) {
            auto prompt_message = enums::visit_indexed(util::overloaded{
                [&](enums::enum_tag_t<play_card_target_type::none>) -> opt_fmt_str {
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(nullptr, nullptr);
                        return std::nullopt;
                    } else {
                        return effect_it->on_prompt(card_ptr, this);
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::player>, int target_id) -> opt_fmt_str {
                    player *target = m_game->find_player(target_id);
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(target, nullptr);
                        return std::nullopt;
                    } else {
                        return effect_it->on_prompt(card_ptr, this, target);
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::other_players>) -> opt_fmt_str {
                    opt_fmt_str msg = std::nullopt;
                    for (auto *p = this;;) {
                        p = m_game->get_next_player(p);
                        if (p == this) {
                            return msg;
                        } else if (!(msg = effect_it->on_prompt(card_ptr, this, p))) {
                            return std::nullopt;
                        }
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::card>, int target_card_id) -> opt_fmt_str {
                    card *target_card = m_game->find_card(target_card_id);
                    player *target = target_card->owner;
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(target, target_card);
                        return std::nullopt;
                    } else {
                        return effect_it->on_prompt(card_ptr, this, target, target_card);
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::cards_other_players>, const std::vector<int> &target_card_ids) -> opt_fmt_str {
                    opt_fmt_str msg = std::nullopt;
                    for (int id : target_card_ids) {
                        card *target_card = m_game->find_card(id);
                        player *target = target_card->owner;
                        if (!(msg = effect_it->on_prompt(card_ptr, this, target, target_card))) {
                            return std::nullopt;
                        }
                    }
                    return msg;
                }
            }, t);
            if (prompt_message) {
                m_game->add_private_update<game_update_type::game_prompt>(this, std::move(*prompt_message));
                m_prompt = std::move(fun);
                return;
            }
            if (++effect_it == effect_end) {
                effect_it = card_ptr->optionals.begin();
                effect_end = card_ptr->optionals.end();
            }
        }

        if (auto prompt_message = card_ptr->multi_target_handler.on_prompt(card_ptr, this, target_list)) {
            m_game->add_private_update<game_update_type::game_prompt>(this, std::move(*prompt_message));
            m_prompt = std::move(fun);
        } else {
            m_game->add_private_update<game_update_type::confirm_play>(this);
            std::invoke(fun);
        }
    }

    void player::check_prompt_equip(card *card_ptr, player *target, std::function<void()> &&fun) {
        if (!card_ptr->equips.empty()) {
            if (auto prompt_message = card_ptr->equips.front().on_prompt(card_ptr, target)) {
                m_game->add_private_update<game_update_type::game_prompt>(this, std::move(*prompt_message));
                m_prompt = std::move(fun);
                return;
            }
        }

        m_game->add_private_update<game_update_type::confirm_play>(this);
        std::invoke(fun);
    }

    void player::do_play_card(card *card_ptr, bool is_response, const std::vector<play_card_target> &targets) {
        assert(card_ptr != nullptr);

        if (m_mandatory_card == card_ptr) {
            m_mandatory_card = nullptr;
        }
        
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        log_played_card(card_ptr, is_response);
        if (std::ranges::find(effects, effect_type::play_card_action, &effect_holder::type) == effects.end()) {
            play_card_action(card_ptr);
        }
        
        auto effect_it = effects.begin();
        auto effect_end = effects.end();

        mth_target_list target_list;
        for (auto &t : targets) {
            enums::visit_indexed(util::overloaded{
                [&](enums::enum_tag_t<play_card_target_type::none>) {
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(nullptr, nullptr);
                    } else {
                        effect_it->on_play(card_ptr, this, effect_flags{});
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::player>, int target_id) {
                    auto *target = m_game->find_player(target_id);
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(target, nullptr);
                    } else {
                        if (target != this && target->immune_to(chosen_card_or(card_ptr))) {
                            if (effect_it->is(effect_type::bangcard)) {
                                request_bang req{card_ptr, this, target};
                                m_game->call_event<event_type::apply_bang_modifier>(this, &req);
                            }
                        } else {
                            auto flags = effect_flags::single_target;
                            if (card_ptr->sign && card_ptr->color == card_color_type::brown && !effect_it->is(effect_type::bangcard)) {
                                flags |= effect_flags::escapable;
                            }
                            effect_it->on_play(card_ptr, this, target, flags);
                        }
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::other_players>) {
                    std::vector<player *> targets;
                    for (auto *p = this;;) {
                        p = m_game->get_next_player(p);
                        if (p == this) break;
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
                        if (!p->immune_to(chosen_card_or(card_ptr))) {
                            effect_it->on_play(card_ptr, this, p, flags);
                        }
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::card>, int target_card_id) {
                    auto *target_card = m_game->find_card(target_card_id);
                    auto *target = target_card->owner;
                    auto flags = effect_flags::single_target;
                    if (card_ptr->sign && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(target, target_card);
                    } else if (target == this) {
                        effect_it->on_play(card_ptr, this, target, target_card, flags);
                    } else if (!target->immune_to(chosen_card_or(card_ptr))) {
                        if (target_card->pocket == pocket_type::player_hand) {
                            effect_it->on_play(card_ptr, this, target, target->random_hand_card(), flags);
                        } else {
                            effect_it->on_play(card_ptr, this, target, target_card, flags);
                        }
                    }
                },
                [&](enums::enum_tag_t<play_card_target_type::cards_other_players>, const std::vector<int> &target_card_ids) {
                    effect_flags flags{};
                    if (card_ptr->sign && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (target_card_ids.size() == 1) {
                        flags |= effect_flags::single_target;
                    }
                    for (int id : target_card_ids) {
                        auto *target_card = m_game->find_card(id);
                        auto *target = target_card->owner;
                        if (target_card->pocket == pocket_type::player_hand) {
                            effect_it->on_play(card_ptr, this, target, target->random_hand_card(), flags);
                        } else {
                            effect_it->on_play(card_ptr, this, target, target_card, flags);
                        }
                    }
                }
            }, t);
            if (++effect_it == effect_end) {
                effect_it = card_ptr->optionals.begin();
                effect_end = card_ptr->optionals.end();
            }
        }

        card_ptr->multi_target_handler.on_play(card_ptr, this, target_list);
        m_game->call_event<event_type::on_effect_end>(this, card_ptr);
    }

    struct confirmer {
        player *p = nullptr;

        ~confirmer() {
            if (std::uncaught_exceptions()) {
                p->m_game->add_private_update<game_update_type::confirm_play>(p);
                if (p->m_forced_card) {
                    p->m_game->add_private_update<game_update_type::force_play_card>(p, p->m_forced_card->id);
                }
            }
        }
    };

    void player::pick_card(const pick_card_args &args) {
        [[maybe_unused]] confirmer _confirm{this};
        m_prompt.reset();
        
        if (m_game->m_requests.empty()) {
            throw game_error("ERROR_INVALID_ACTION");
        }

        auto &req = m_game->top_request();
        if (req.target() != this) {
            throw game_error("ERROR_INVALID_ACTION");
        }

        m_game->add_private_update<game_update_type::confirm_play>(this);
        player *target_player = args.player_id ? m_game->find_player(args.player_id) : nullptr;
        card *target_card = args.card_id ? m_game->find_card(args.card_id) : nullptr;
        if (req.can_pick(args.pocket, target_player, target_card)) {
            req.on_pick(args.pocket, target_player, target_card);
        }
    }

    void player::play_card(const play_card_args &args) {
        [[maybe_unused]] confirmer _confirm{this};
        m_prompt.reset();

        if (!m_game->m_requests.empty() || m_game->m_playing != this) {
            throw game_error("ERROR_INVALID_ACTION");
        }
        
        std::vector<card *> modifiers;
        for (int id : args.modifier_ids) {
            modifiers.push_back(m_game->find_card(id));
        }

        card *card_ptr = m_game->find_card(args.card_id);

        if (m_mandatory_card == card_ptr) {
            m_mandatory_card = nullptr;
        }
        
        if (m_forced_card) {
            if (card_ptr != m_forced_card && std::ranges::find(modifiers, m_forced_card) == modifiers.end()) {
                throw game_error("ERROR_INVALID_ACTION");
            } else {
                m_forced_card = nullptr;
            }
        }

        switch(card_ptr->pocket) {
        case pocket_type::player_hand:
            if (!modifiers.empty() && modifiers.front()->modifier == card_modifier_type::leevankliff) {
                // Uso il raii eliminare il limite di bang
                // quando lee van kliff gioca l'effetto del personaggio su una carta bang.
                // Se le funzioni di verifica throwano viene chiamato il distruttore
                struct banglimit_remover {
                    int8_t &num;
                    banglimit_remover(int8_t &num) : num(num) { ++num; }
                    ~banglimit_remover() { --num; }
                } _banglimit_remover{m_bangs_per_turn};
                if (m_game->is_disabled(modifiers.front())) throw game_error("ERROR_CARD_IS_DISABLED", modifiers.front());
                verify_card_targets(m_last_played_card, false, args.targets);
                check_prompt(m_last_played_card, false, args.targets, [=, this]{
                    m_game->move_to(card_ptr, pocket_type::discard_pile);
                    m_game->call_event<event_type::on_play_hand_card>(this, card_ptr);
                    do_play_card(m_last_played_card, false, args.targets);
                    set_last_played_card(nullptr);
                });
            } else if (card_ptr->color == card_color_type::brown) {
                if (m_game->is_disabled(card_ptr)) throw game_error("ERROR_CARD_IS_DISABLED", card_ptr);
                verify_modifiers(card_ptr, modifiers);
                verify_card_targets(card_ptr, false, args.targets);
                check_prompt(card_ptr, false, args.targets, [=, this]{
                    play_modifiers(modifiers);
                    do_play_card(card_ptr, false, args.targets);
                    set_last_played_card(card_ptr);
                });
            } else {
                if (m_game->has_scenario(scenario_flags::judge)) throw game_error("ERROR_CANT_EQUIP_CARDS");
                verify_equip_target(card_ptr, args.targets);
                auto *target = m_game->find_player(args.targets.front().get<play_card_target_type::player>());
                if (auto *card = target->find_equipped_card(card_ptr)) throw game_error("ERROR_DUPLICATED_CARD", card);
                if (card_ptr->color == card_color_type::orange && m_game->m_cubes.size() < 3) {
                    throw game_error("ERROR_NOT_ENOUGH_CUBES");
                }
                check_prompt_equip(card_ptr, target, [=, this]{
                    if (target != this && target->immune_to(card_ptr)) {
                        discard_card(card_ptr);
                    } else {
                        target->equip_card(card_ptr);
                        if (this == target) {
                            m_game->add_log("LOG_EQUIPPED_CARD", card_ptr, this);
                        } else {
                            m_game->add_log("LOG_EQUIPPED_CARD_TO", card_ptr, this, target);
                        }
                        switch (card_ptr->color) {
                        case card_color_type::blue:
                            if (m_game->has_expansion(card_expansion_type::armedanddangerous) && can_receive_cubes()) {
                                m_game->queue_request<request_add_cube>(card_ptr, this);
                            }
                            break;
                        case card_color_type::green:
                            card_ptr->inactive = true;
                            m_game->add_public_update<game_update_type::tap_card>(card_ptr->id, true);
                            break;
                        case card_color_type::orange:
                            add_cubes(card_ptr, 3);
                            break;
                        }
                        m_game->call_event<event_type::on_equip>(this, target, card_ptr);
                    }
                    set_last_played_card(nullptr);
                    m_game->call_event<event_type::on_effect_end>(this, card_ptr);
                });
            }
            break;
        case pocket_type::player_character:
        case pocket_type::player_table:
            if (m_game->is_disabled(card_ptr)) throw game_error("ERROR_CARD_IS_DISABLED", card_ptr);
            if (card_ptr->inactive) throw game_error("ERROR_CARD_INACTIVE", card_ptr);
            verify_modifiers(card_ptr, modifiers);
            verify_card_targets(card_ptr, false, args.targets);
            check_prompt(card_ptr, false, args.targets, [=, this]{
                play_modifiers(modifiers);
                do_play_card(card_ptr, false, args.targets);
                set_last_played_card(nullptr);
            });
            break;
        case pocket_type::hidden_deck:
            if (std::ranges::find(modifiers, card_modifier_type::shopchoice, &card::modifier) == modifiers.end()) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            [[fallthrough]];
        case pocket_type::shop_selection: {
            int cost = card_ptr->buy_cost();
            for (card *c : modifiers) {
                if (m_game->is_disabled(c)) throw game_error("ERROR_CARD_IS_DISABLED", c);
                switch (c->modifier) {
                case card_modifier_type::discount:
                    if (c->usages) throw game_error("ERROR_MAX_USAGES", c, 1);
                    --cost;
                    break;
                case card_modifier_type::shopchoice:
                    if (c->effects.front().type != card_ptr->effects.front().type) throw game_error("ERROR_INVALID_ACTION");
                    cost += c->buy_cost();
                    break;
                }
            }
            if (m_game->m_shop_selection.size() > 3) {
                cost = 0;
            }
            if (m_gold < cost) throw game_error("ERROR_NOT_ENOUGH_GOLD");
            if (card_ptr->color == card_color_type::brown) {
                verify_card_targets(card_ptr, false, args.targets);
                check_prompt(card_ptr, false, args.targets, [=, this]{
                    play_modifiers(modifiers);
                    add_gold(-cost);
                    do_play_card(card_ptr, false, args.targets);
                    set_last_played_card(nullptr);
                });
            } else {
                if (m_game->has_scenario(scenario_flags::judge)) throw game_error("ERROR_CANT_EQUIP_CARDS");
                verify_equip_target(card_ptr, args.targets);
                auto *target = m_game->find_player(args.targets.front().get<play_card_target_type::player>());
                if (card *card = target->find_equipped_card(card_ptr)) throw game_error("ERROR_DUPLICATED_CARD", card);
                check_prompt_equip(card_ptr, target, [=, this]{
                    play_modifiers(modifiers);
                    add_gold(-cost);
                    target->equip_card(card_ptr);
                    set_last_played_card(nullptr);
                    if (this == target) {
                        m_game->add_log("LOG_BOUGHT_EQUIP", card_ptr, this);
                    } else {
                        m_game->add_log("LOG_BOUGHT_EQUIP_TO", card_ptr, this, target);
                    }
                    m_game->call_event<event_type::on_effect_end>(this, card_ptr);
                });
            }
            m_game->queue_action([&]{
                while (m_game->m_shop_selection.size() < 3) {
                    m_game->draw_shop_card();
                }
            });
            break;
        }
        case pocket_type::scenario_card:
        case pocket_type::specials:
            verify_card_targets(card_ptr, false, args.targets);
            check_prompt(card_ptr, false, args.targets, [=, this]{
                do_play_card(card_ptr, false, args.targets);
                set_last_played_card(nullptr);
            });
            break;
        default:
            throw game_error("play_card: invalid card"_nonloc);
        }
    }
    
    void player::respond_card(const play_card_args &args) {
        [[maybe_unused]] confirmer _confirm{this};
        m_prompt.reset();
        
        card *card_ptr = m_game->find_card(args.card_id);

        if (!can_respond_with(card_ptr)) throw game_error("ERROR_INVALID_ACTION");
        
        if (m_forced_card) {
            if (card_ptr != m_forced_card) {
                throw game_error("ERROR_INVALID_ACTION");
            } else {
                m_forced_card = nullptr;
            }
        }

        switch (card_ptr->pocket) {
        case pocket_type::player_table:
            if (card_ptr->inactive) throw game_error("ERROR_CARD_INACTIVE", card_ptr);
            break;
        case pocket_type::player_hand:
            if (card_ptr->color != card_color_type::brown) throw game_error("INVALID_ACTION");
            break;
        case pocket_type::player_character:
        case pocket_type::scenario_card:
        case pocket_type::specials:
            break;
        default:
            throw game_error("respond_card: invalid card"_nonloc);
        }
        
        verify_card_targets(card_ptr, true, args.targets);
        check_prompt(card_ptr, true, args.targets, [=, this]{
            do_play_card(card_ptr, true, args.targets);
            set_last_played_card(nullptr);
        });
    }

    void player::prompt_response(bool response) {
        if (!m_prompt) {
            throw game_error("ERROR_INVALID_ACTION");
        }

        m_game->add_private_update<game_update_type::confirm_play>(this);
        if (response) {
            std::invoke(*m_prompt);
        }
        m_prompt.reset();
    }

    void player::draw_from_deck() {
        int save_numcards = m_num_cards_to_draw;
        m_game->call_event<event_type::on_draw_from_deck>(this);
        if (m_game->pop_request<request_draw>()) {
            m_game->add_log("LOG_DRAWN_FROM_DECK", this);
            while (m_num_drawn_cards<m_num_cards_to_draw) {
                ++m_num_drawn_cards;
                card *drawn_card = m_game->draw_phase_one_card_to(pocket_type::player_hand, this);
                m_game->add_log("LOG_DRAWN_CARD", this, drawn_card);
                m_game->call_event<event_type::on_card_drawn>(this, drawn_card);
            }
        }
        m_num_cards_to_draw = save_numcards;
        m_game->queue_action([this]{
            m_game->call_event<event_type::post_draw_cards>(this);
        });
    }

    card_sign player::get_card_sign(card *target_card) {
        card_sign sign = target_card->sign;
        m_game->call_event<event_type::apply_sign_modifier>(this, sign);
        return sign;
    }

    void player::start_of_turn() {
        if (this != m_game->m_playing && this == m_game->m_first_player) {
            m_game->draw_scenario_card();
        }
        
        m_game->m_playing = this;

        m_mandatory_card = nullptr;
        m_bangs_played = 0;
        m_bangs_per_turn = 1;
        m_num_drawn_cards = 0;
        add_player_flags(player_flags::start_of_turn);

        if (!check_player_flags(player_flags::ghost) && m_hp == 0) {
            if (m_game->has_scenario(scenario_flags::ghosttown)) {
                ++m_num_cards_to_draw;
                for (auto *c : m_characters) {
                    equip_if_enabled(c);
                }
            } else if (m_game->has_scenario(scenario_flags::deadman) && this == m_game->m_first_dead) {
                remove_player_flags(player_flags::dead);
                m_game->add_public_update<game_update_type::player_hp>(id, m_hp = 2);
                m_game->draw_card_to(pocket_type::player_hand, this);
                m_game->draw_card_to(pocket_type::player_hand, this);
                for (auto *c : m_characters) {
                    equip_if_enabled(c);
                }
            }
        }
        
        for (card *c : m_characters) {
            c->usages = 0;
        }
        for (card *c : m_table) {
            c->usages = 0;
        }
        for (auto &[card_id, obj] : m_predraw_checks) {
            obj.resolved = false;
        }
        
        m_game->add_public_update<game_update_type::switch_turn>(id);
        m_game->add_log("LOG_TURN_START", this);
        m_game->call_event<event_type::pre_turn_start>(this);
        next_predraw_check(nullptr);
    }

    void player::next_predraw_check(card *target_card) {
        if (auto it = m_predraw_checks.find(target_card); it != m_predraw_checks.end()) {
            it->second.resolved = true;
        }
        m_game->queue_action([this]{
            if (alive() && m_game->m_playing == this && !m_game->m_game_over) {
                if (std::ranges::all_of(m_predraw_checks | std::views::values, &predraw_check::resolved)) {
                    request_drawing();
                } else {
                    m_game->queue_request<request_predraw>(this);
                }
            }
        });
    }

    void player::request_drawing() {
        m_game->call_event<event_type::on_turn_start>(this);
        m_game->queue_action([this]{
            m_game->call_event<event_type::on_request_draw>(this);
            if (m_game->m_requests.empty()) {
                m_game->queue_request<request_draw>(this);
            }
        });
    }

    void player::verify_pass_turn() {
        if (m_mandatory_card && m_mandatory_card->owner == this && is_possible_to_play(m_mandatory_card)) {
            throw game_error("ERROR_MANDATORY_CARD", m_mandatory_card);
        }
    }

    void player::pass_turn() {
        m_mandatory_card = nullptr;
        if (m_hand.size() > max_cards_end_of_turn()) {
            m_game->queue_request<request_discard_pass>(this);
        } else {
            untap_inactive_cards();

            m_game->call_event<event_type::on_turn_end>(this);
            if (!check_player_flags(player_flags::extra_turn)) {
                m_game->call_event<event_type::post_turn_end>(this);
            }
            m_game->queue_action([&]{
                if (m_extra_turns == 0) {
                    if (!check_player_flags(player_flags::ghost) && m_hp == 0 && m_game->has_scenario(scenario_flags::ghosttown)) {
                        --m_num_cards_to_draw;
                        m_game->player_death(nullptr, this);
                    }
                    remove_player_flags(player_flags::extra_turn);
                    m_game->get_next_in_turn(this)->start_of_turn();
                } else {
                    --m_extra_turns;
                    add_player_flags(player_flags::extra_turn);
                    start_of_turn();
                }
            });
        }
    }

    void player::skip_turn() {
        untap_inactive_cards();
        remove_player_flags(player_flags::extra_turn);
        m_game->call_event<event_type::on_turn_end>(this);
        m_game->get_next_in_turn(this)->start_of_turn();
    }

    void player::untap_inactive_cards() {
        for (card *c : m_table) {
            if (c->inactive) {
                c->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(c->id, false);
            }
        }
    }

    void player::discard_all() {
        while (!m_table.empty()) {
            discard_card(m_table.front());
        }
        drop_all_cubes(m_characters.front());
        while (!m_hand.empty()) {
            m_game->move_to(m_hand.front(), pocket_type::discard_pile);
        }
    }

    void player::set_role(player_role role) {
        m_role = role;

        if (role == player_role::sheriff || m_game->m_players.size() <= 3) {
            m_game->add_public_update<game_update_type::player_show_role>(id, m_role, true);
            add_player_flags(player_flags::role_revealed);
        } else {
            m_game->add_private_update<game_update_type::player_show_role>(this, id, m_role, true);
        }
    }

    void player::send_player_status() {
        m_game->add_public_update<game_update_type::player_status>(id, m_player_flags, m_range_mod, m_weapon_range, m_distance_mod);
    }

    void player::add_player_flags(player_flags flags) {
        if (!check_player_flags(flags)) {
            m_player_flags |= flags;
            send_player_status();
        }
    }

    void player::remove_player_flags(player_flags flags) {
        if (check_player_flags(flags)) {
            m_player_flags &= ~flags;
            send_player_status();
        }
    }

    bool player::check_player_flags(player_flags flags) const {
        return (m_player_flags & flags) == flags;
    }

    int player::count_cubes() const {
        return m_characters.front()->cubes.size() + std::transform_reduce(m_table.begin(), m_table.end(), 0,
            std::plus(), [](const card *c) { return c->cubes.size(); });
    }
}