#include "player.h"

#include "game.h"

#include "holders.h"
#include "server/net_enums.h"

#include <cassert>

namespace banggame {
    using namespace enums::flag_operators;

    player::player(game *game)
        : m_game(game)
        , id(game->get_next_id()) {}

    void player::equip_card(card *target) {
        for (auto &e : target->equips) {
            e.on_pre_equip(target, this);
        }

        m_game->move_to(target, card_pile_type::player_table, true, this, show_card_flags::show_everyone);
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
        m_game->instant_event<event_type::apply_initial_cards_modifier>(this, value);
        return value;
    }

    int player::max_cards_end_of_turn() {
        int n = 0;
        m_game->instant_event<event_type::apply_maxcards_modifier>(this, n);
        if (!n) n = m_hp;
        m_game->instant_event<event_type::apply_maxcards_adder>(this, n);
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

    std::vector<card *>::iterator player::move_card_to(card *target_card, card_pile_type pile, bool known, player *owner, show_card_flags flags) {
        if (target_card->owner != this) throw game_error("ERROR_PLAYER_DOES_NOT_OWN_CARD");
        if (target_card->pile == card_pile_type::player_table) {
            if (target_card->inactive) {
                target_card->inactive = false;
                m_game->add_public_update<game_update_type::tap_card>(target_card->id, false);
            }
            drop_all_cubes(target_card);
            auto it = m_game->move_to(target_card, pile, known, owner, flags);
            m_game->queue_event<event_type::post_discard_card>(this, target_card);
            unequip_if_enabled(target_card);
            return it;
        } else if (target_card->pile == card_pile_type::player_hand) {
            return m_game->move_to(target_card, pile, known, owner, flags);
        } else {
            throw game_error("ERROR_CARD_NOT_FOUND");
        }
    }

    void player::discard_card(card *target) {
        move_card_to(target, target->color == card_color_type::black
            ? card_pile_type::shop_discard
            : card_pile_type::discard_pile, true);
    }

    void player::steal_card(player *target, card *target_card) {
        target->move_card_to(target_card, card_pile_type::player_hand, true, this);
    }

    void player::damage(card *origin_card, player *origin, int value, bool is_bang, bool instant) {
        if (!check_player_flags(player_flags::ghost) && !(m_hp == 0 && m_game->has_scenario(scenario_flags::ghosttown))) {
            if (instant || !m_game->has_expansion(card_expansion_type::valleyofshadows | card_expansion_type::canyondiablo)) {
                m_hp -= value;
                m_game->add_public_update<game_update_type::player_hp>(id, m_hp);
                m_game->add_log(value == 1 ? "LOG_TAKEN_DAMAGE" : "LOG_TAKEN_DAMAGE_PLURAL", origin_card, this, value);
                if (m_hp <= 0) {
                    m_game->add_request<request_type::death>(origin_card, origin, this);
                }
                if (m_game->has_expansion(card_expansion_type::goldrush)) {
                    if (origin && origin->m_game->m_playing == origin && origin != this) {
                        origin->add_gold(value);
                    }
                }
                m_game->queue_event<event_type::on_hit>(origin_card, origin, this, value, is_bang);
            } else {
                m_game->add_request<request_type::damaging>(origin_card, origin, this, value, is_bang);
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
        m_game->instant_event<event_type::apply_immunity_modifier>(c, this, value);
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
        // controlla se e' necessario attivare request di fuga
        if (bool(flags & effect_flags::escapable)
            && m_game->has_expansion(card_expansion_type::valleyofshadows)) return true;
        auto it = std::ranges::find_if(m_characters, [](const auto &vec) {
            return !vec.empty() && vec.front().type == effect_type::ms_abigail;
        }, &card::responses);
        return it != m_characters.end()
            && (*it)->responses.front().get<effect_type::ms_abigail>().can_escape(origin, origin_card, flags);
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
        if (target->cubes.empty() && target->pile != card_pile_type::player_character && target->pile != card_pile_type::player_backup) {
            owner->m_game->move_to(target, card_pile_type::discard_pile);
            owner->m_game->instant_event<event_type::post_discard_orange_card>(owner, target);
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
        m_game->move_to(target, card_pile_type::player_hand, true, this);
    }

    void player::set_last_played_card(card *c) {
        m_last_played_card = c;
        m_game->add_private_update<game_update_type::last_played_card>(this, c ? c->id : 0);
    }

    void player::set_forced_card(card *c) {
        m_forced_card = c;
        m_game->add_private_update<game_update_type::force_play_card>(this, c ? c->id : 0);
    }

    void player::verify_modifiers(card *c, const std::vector<card *> &modifiers) {
        for (card *mod_card : modifiers) {
            card_suit_type suit = get_card_suit(mod_card);
            if (m_game->m_playing == this && m_declared_suit != card_suit_type::none && suit != card_suit_type::none && suit != m_declared_suit) {
                throw game_error("ERROR_WRONG_DECLARED_SUIT");
            }

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
                play_card_target{enums::enum_constant<play_card_target_type::none>{}}});
        }
    }

    void player::verify_equip_target(card *c, const std::vector<play_card_target> &targets) {
        card_suit_type suit = get_card_suit(c);
        if (m_game->m_playing == this && m_declared_suit != card_suit_type::none && suit != card_suit_type::none && suit != m_declared_suit) {
            throw game_error("ERROR_WRONG_DECLARED_SUIT");
        }

        if (c->equips.empty()) return;
        if (targets.size() != 1) throw game_error("ERROR_INVALID_ACTION");
        if (targets.front().enum_index() != play_card_target_type::player) throw game_error("ERROR_INVALID_ACTION");
        int target_player_id = targets.front().get<play_card_target_type::player>();
        verify_effect_player_target(c->equips.front().player_filter, m_game->get_player(target_player_id));
    }

    bool player::is_bangcard(card *card_ptr) {
        return check_player_flags(player_flags::treat_any_as_bang)
            || (check_player_flags(player_flags::treat_missed_as_bang)
                && !card_ptr->responses.empty() && card_ptr->responses.front().is(effect_type::missedcard))
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
                if (target_card->pile != card_pile_type::player_table) {
                    throw game_error("ERROR_TARGET_NOT_TABLE_CARD");
                }
                break;
            case target_card_filter::hand:
                if (target_card->pile != card_pile_type::player_hand) {
                    throw game_error("ERROR_TARGET_NOT_HAND_CARD");
                }
                break;
            case target_card_filter::blue:
                if (target_card->color != card_color_type::blue) {
                    throw game_error("ERROR_TARGET_NOT_BLUE_CARD");
                }
                break;
            case target_card_filter::clubs:
                if (get_card_suit(target_card) != card_suit_type::clubs) {
                    throw game_error("ERROR_TARGET_NOT_CLUBS");
                }
                break;
            case target_card_filter::bang:
                if (!is_bangcard(target_card)) {
                    throw game_error("ERROR_TARGET_NOT_BANG");
                }
                break;
            case target_card_filter::missed:
                if (target_card->responses.empty() || !target_card->responses.front().is(effect_type::missedcard)) {
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
        card_suit_type suit = get_card_suit(m_chosen_card ? m_chosen_card : card_ptr);
        if (m_game->m_playing == this && m_declared_suit != card_suit_type::none && suit != card_suit_type::none && suit != m_declared_suit) {
            throw game_error("ERROR_WRONG_DECLARED_SUIT");
        }

        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;

        int diff = targets.size() - effects.size();
        if (!card_ptr->optionals.empty() && card_ptr->optionals.back().is(effect_type::repeatable)) {
            if (diff < 0 || diff % card_ptr->optionals.size() != 0
                || (card_ptr->optionals.back().args > 0
                    && diff > (card_ptr->optionals.size() * card_ptr->optionals.back().args)))
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
                [&](enums::enum_constant<play_card_target_type::none>) {
                    if (e.target != play_card_target_type::none) throw game_error("ERROR_INVALID_ACTION");
                    if (e.is(effect_type::mth_add)) target_list.emplace_back(nullptr, nullptr);

                    e.verify(card_ptr, this);
                },
                [&](enums::enum_constant<play_card_target_type::player>, int target_id) {
                    if (e.target != play_card_target_type::player) throw game_error("ERROR_INVALID_ACTION");

                    player *target = m_game->get_player(target_id);

                    verify_effect_player_target(e.player_filter, target);
                    e.verify(card_ptr, this, target);

                    if (e.is(effect_type::mth_add)) target_list.emplace_back(target, nullptr);
                },
                [&](enums::enum_constant<play_card_target_type::card>, int target_card_id) {
                    if (e.target != play_card_target_type::card) throw game_error("ERROR_INVALID_ACTION");

                    card *target_card = m_game->find_card(target_card_id);
                    player *target = target_card->owner;
                    if (!target) throw game_error("ERROR_INVALID_ACTION");

                    verify_effect_card_target(e, target, target_card);
                    e.verify(card_ptr, this, target, target_card);

                    if (e.is(effect_type::mth_add)) target_list.emplace_back(target, target_card);
                },
                [&](enums::enum_constant<play_card_target_type::other_players>) {
                    if (e.target != play_card_target_type::other_players) throw game_error("ERROR_INVALID_ACTION");
                    for (auto *p = this;;) {
                        p = m_game->get_next_player(p);
                        if (p == this) break;
                        e.verify(card_ptr, this, p);
                    }
                },
                [&](enums::enum_constant<play_card_target_type::cards_other_players>, const std::vector<int> &target_ids) {
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

        if (card_ptr->multi_target_handler != mth_type::none) {
            enums::visit_enum([&](auto enum_const) {
                constexpr mth_type E = decltype(enum_const)::value;
                if constexpr (enums::has_type<E>) {
                    using handler_type = enums::enum_type_t<E>;
                    if constexpr (requires (handler_type handler) { handler.verify(card_ptr, this, target_list); }) {
                        handler_type{}.verify(card_ptr, this, std::move(target_list));
                    }
                }
            }, card_ptr->multi_target_handler);
        }
    }

    void player::play_card_action(card *card_ptr, bool is_response) {
        switch (card_ptr->pile) {
        case card_pile_type::player_hand:
            m_game->move_to(card_ptr, card_pile_type::discard_pile);
            m_game->queue_event<event_type::on_play_hand_card>(this, card_ptr);
            [[fallthrough]];
        case card_pile_type::scenario_card:
            m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CARD" : "LOG_PLAYED_CARD", card_ptr, this);
            break;
        case card_pile_type::player_table:
            if (card_ptr->color == card_color_type::green) {
                m_game->move_to(card_ptr, card_pile_type::discard_pile);
            }
            m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CARD" : "LOG_PLAYED_TABLE_CARD", card_ptr, this);
            break;
        case card_pile_type::player_character:
            m_game->add_log(is_response ? "LOG_RESPONDED_WITH_CHARACTER" : "LOG_PLAYED_CHARACTER", card_ptr, this);
            break;
        case card_pile_type::shop_selection:
            if (card_ptr->color == card_color_type::brown) {
                m_game->move_to(card_ptr, card_pile_type::shop_discard);
            }
            m_game->add_log("LOG_BOUGHT_CARD", card_ptr, this);
            break;
        case card_pile_type::hidden_deck:
            break;
        }
    }

    void player::do_play_card(card *card_ptr, bool is_response, const std::vector<play_card_target> &targets) {
        assert(card_ptr != nullptr);
        
        auto &effects = is_response ? card_ptr->responses : card_ptr->effects;
        if (std::ranges::find(effects, effect_type::play_card_action, &effect_holder::type) == effects.end()) {
            play_card_action(card_ptr, is_response);
        }

        auto check_immunity = [&](player *target) {
            return target->immune_to(m_chosen_card ? m_chosen_card : card_ptr);
        };
        
        auto effect_it = effects.begin();
        auto effect_end = effects.end();

        mth_target_list target_list;
        for (auto &t : targets) {
            enums::visit_indexed(util::overloaded{
                [&](enums::enum_constant<play_card_target_type::none>) {
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(nullptr, nullptr);
                    } else {
                        effect_it->on_play(card_ptr, this, effect_flags{});
                    }
                },
                [&](enums::enum_constant<play_card_target_type::player>, int target_id) {
                    auto *target = m_game->get_player(target_id);
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(target, nullptr);
                    } else {
                        if (target != this && check_immunity(target)) {
                            if (effect_it->is(effect_type::bangcard)) {
                                request_bang req{card_ptr, this, target};
                                apply_bang_mods(req);
                                req.cleanup();
                            }
                        } else {
                            auto flags = effect_flags::single_target;
                            if (card_ptr->value != card_value_type::none && card_ptr->color == card_color_type::brown && !effect_it->is(effect_type::bangcard)) {
                                flags |= effect_flags::escapable;
                            }
                            effect_it->on_play(card_ptr, this, target, flags);
                        }
                    }
                },
                [&](enums::enum_constant<play_card_target_type::other_players>) {
                    std::vector<player *> targets;
                    for (auto *p = this;;) {
                        p = m_game->get_next_player(p);
                        if (p == this) break;
                        targets.push_back(p);
                    }
                    
                    effect_flags flags{};
                    if (card_ptr->value != card_value_type::none && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (targets.size() == 1) {
                        flags |= effect_flags::single_target;
                    }
                    for (auto *p : targets) {
                        if (!check_immunity(p)) {
                            effect_it->on_play(card_ptr, this, p, flags);
                        }
                    }
                },
                [&](enums::enum_constant<play_card_target_type::card>, int target_card_id) {
                    auto *target_card = m_game->find_card(target_card_id);
                    auto *target = target_card->owner;
                    auto flags = effect_flags::single_target;
                    if (card_ptr->value != card_value_type::none && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (effect_it->is(effect_type::mth_add)) {
                        target_list.emplace_back(target, target_card);
                    } else if (target == this) {
                        effect_it->on_play(card_ptr, this, target, target_card, flags);
                    } else if (!check_immunity(target)) {
                        if (target_card->pile == card_pile_type::player_hand) {
                            effect_it->on_play(card_ptr, this, target, target->random_hand_card(), flags);
                        } else {
                            effect_it->on_play(card_ptr, this, target, target_card, flags);
                        }
                    }
                },
                [&](enums::enum_constant<play_card_target_type::cards_other_players>, const std::vector<int> &target_card_ids) {
                    effect_flags flags{};
                    if (card_ptr->value != card_value_type::none && card_ptr->color == card_color_type::brown) {
                        flags |= effect_flags::escapable;
                    }
                    if (target_card_ids.size() == 1) {
                        flags |= effect_flags::single_target;
                    }
                    for (int id : target_card_ids) {
                        auto *target_card = m_game->find_card(id);
                        auto *target = target_card->owner;
                        if (target_card->pile == card_pile_type::player_hand) {
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

        if (card_ptr->multi_target_handler != mth_type::none) {
            enums::visit_enum([&](auto enum_const) {
                constexpr mth_type E = decltype(enum_const)::value;
                if constexpr (enums::has_type<E>) {
                    using handler_type = enums::enum_type_t<E>;
                    handler_type{}.on_play(card_ptr, this, std::move(target_list));
                }
            }, card_ptr->multi_target_handler);
        }
        m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
    }

    struct confirmer {
        player *p = nullptr;

        confirmer(player *p) : p(p) {
            p->m_game->add_private_update<game_update_type::confirm_play>(p);
        }

        ~confirmer() {
            if (p->m_forced_card && std::uncaught_exceptions()) {
                p->m_game->add_private_update<game_update_type::force_play_card>(p, p->m_forced_card->id);
            }
        }
    };

    void player::play_card(const play_card_args &args) {
        confirmer _confirm{this};
        
        std::vector<card *> modifiers;
        for (int id : args.modifier_ids) {
            modifiers.push_back(m_game->find_card(id));
        }

        card *card_ptr = m_game->find_card(args.card_id);
        
        bool was_forced_card = false;
        if (m_forced_card) {
            if (card_ptr != m_forced_card && std::ranges::find(modifiers, m_forced_card) == modifiers.end()) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            was_forced_card = true;
        }

        switch(card_ptr->pile) {
        case card_pile_type::player_hand:
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
                m_game->move_to(card_ptr, card_pile_type::discard_pile);
                m_game->queue_event<event_type::on_play_hand_card>(this, card_ptr);
                do_play_card(m_last_played_card, false, args.targets);
                set_last_played_card(nullptr);
            } else if (card_ptr->color == card_color_type::brown) {
                if (m_game->is_disabled(card_ptr)) throw game_error("ERROR_CARD_IS_DISABLED", card_ptr);
                verify_modifiers(card_ptr, modifiers);
                verify_card_targets(card_ptr, false, args.targets);
                play_modifiers(modifiers);
                do_play_card(card_ptr, false, args.targets);
                set_last_played_card(card_ptr);
            } else {
                if (m_game->has_scenario(scenario_flags::judge)) throw game_error("ERROR_CANT_EQUIP_CARDS");
                verify_equip_target(card_ptr, args.targets);
                auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::player>());
                if (auto *card = target->find_equipped_card(card_ptr)) throw game_error("ERROR_DUPLICATED_CARD", card);
                if (card_ptr->color == card_color_type::orange && m_game->m_cubes.size() < 3) {
                    throw game_error("ERROR_NOT_ENOUGH_CUBES");
                }
                if (target != this && target->immune_to(card_ptr)) {
                    discard_card(card_ptr);
                } else {
                    target->equip_card(card_ptr);
                    m_game->queue_event<event_type::on_equip>(this, target, card_ptr);
                    if (this == target) {
                        m_game->add_log("LOG_EQUIPPED_CARD", card_ptr, this);
                    } else {
                        m_game->add_log("LOG_EQUIPPED_CARD_TO", card_ptr, this, target);
                    }
                    switch (card_ptr->color) {
                    case card_color_type::blue:
                        if (m_game->has_expansion(card_expansion_type::armedanddangerous) && can_receive_cubes()) {
                            m_game->queue_request<request_type::add_cube>(card_ptr, this);
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
                }
                set_last_played_card(nullptr);
                m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
            }
            break;
        case card_pile_type::player_character:
        case card_pile_type::player_table:
            if (m_game->is_disabled(card_ptr)) throw game_error("ERROR_CARD_IS_DISABLED", card_ptr);
            if (card_ptr->inactive) throw game_error("ERROR_CARD_INACTIVE", card_ptr);
            verify_modifiers(card_ptr, modifiers);
            verify_card_targets(card_ptr, false, args.targets);
            play_modifiers(modifiers);
            do_play_card(card_ptr, false, args.targets);
            set_last_played_card(nullptr);
            break;
        case card_pile_type::hidden_deck:
            if (std::ranges::find(modifiers, card_modifier_type::shopchoice, &card::modifier) == modifiers.end()) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            [[fallthrough]];
        case card_pile_type::shop_selection: {
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
            if (was_forced_card) {
                cost = 0;
            }
            if (m_gold < cost) throw game_error("ERROR_NOT_ENOUGH_GOLD");
            if (card_ptr->color == card_color_type::brown) {
                verify_card_targets(card_ptr, false, args.targets);
                play_modifiers(modifiers);
                add_gold(-cost);
                do_play_card(card_ptr, false, args.targets);
                set_last_played_card(card_ptr);
                m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
            } else {
                if (m_game->has_scenario(scenario_flags::judge)) throw game_error("ERROR_CANT_EQUIP_CARDS");
                verify_equip_target(card_ptr, args.targets);
                play_modifiers(modifiers);
                auto *target = m_game->get_player(args.targets.front().get<play_card_target_type::player>());
                if (card *card = target->find_equipped_card(card_ptr)) throw game_error("ERROR_DUPLICATED_CARD", card);
                add_gold(-cost);
                target->equip_card(card_ptr);
                set_last_played_card(nullptr);
                if (this == target) {
                    m_game->add_log("LOG_BOUGHT_EQUIP", card_ptr, this);
                } else {
                    m_game->add_log("LOG_BOUGHT_EQUIP_TO", card_ptr, this, target);
                }
                m_game->queue_event<event_type::on_effect_end>(this, card_ptr);
            }
            m_game->queue_delayed_action([&]{
                while (m_game->m_shop_selection.size() < 3) {
                    m_game->draw_shop_card();
                }
            });
            break;
        }
        case card_pile_type::scenario_card:
        case card_pile_type::specials:
            verify_card_targets(card_ptr, false, args.targets);
            do_play_card(card_ptr, false, args.targets);
            set_last_played_card(nullptr);
            break;
        default:
            throw game_error("play_card: invalid card"_nonloc);
        }
        remove_player_flags(player_flags::start_of_turn);
        if (was_forced_card) {
            m_forced_card = nullptr;
        }
    }
    
    void player::respond_card(const play_card_args &args) {
        confirmer _confirm{this};
        
        card *card_ptr = m_game->find_card(args.card_id);

        if (!can_respond_with(card_ptr)) throw game_error("ERROR_INVALID_ACTION");
        
        bool was_forced_card = false;
        if (m_forced_card) {
            if (card_ptr != m_forced_card) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            was_forced_card = true;
        }

        switch (card_ptr->pile) {
        case card_pile_type::player_table:
            if (card_ptr->inactive) throw game_error("ERROR_CARD_INACTIVE", card_ptr);
            break;
        case card_pile_type::player_hand:
            if (card_ptr->color != card_color_type::brown) throw game_error("INVALID_ACTION");
            break;
        case card_pile_type::player_character:
        case card_pile_type::scenario_card:
        case card_pile_type::specials:
            break;
        default:
            throw game_error("respond_card: invalid card"_nonloc);
        }
        
        verify_card_targets(card_ptr, true, args.targets);
        do_play_card(card_ptr, true, args.targets);
        set_last_played_card(nullptr);
        if (was_forced_card) {
            m_forced_card = nullptr;
        }
    }

    void player::draw_from_deck() {
        int save_numcards = m_num_cards_to_draw;
        m_game->instant_event<event_type::on_draw_from_deck>(this);
        if (m_game->top_request_is(request_type::draw)) {
            m_game->pop_request(request_type::draw);
            m_game->add_log("LOG_DRAWN_FROM_DECK", this);
            while (m_num_drawn_cards<m_num_cards_to_draw) {
                ++m_num_drawn_cards;
                card *drawn_card = m_game->draw_phase_one_card_to(card_pile_type::player_hand, this);
                m_game->add_log("LOG_DRAWN_CARD", this, drawn_card);
                m_game->instant_event<event_type::on_card_drawn>(this, drawn_card);
            }
        }
        m_num_cards_to_draw = save_numcards;
        m_game->queue_event<event_type::post_draw_cards>(this);
    }

    card_suit_type player::get_card_suit(card *drawn_card) {
        card_suit_type suit = drawn_card->suit;
        m_game->instant_event<event_type::apply_suit_modifier>(suit);
        return suit;
    }

    card_value_type player::get_card_value(card *drawn_card) {
        card_value_type value = drawn_card->value;
        m_game->instant_event<event_type::apply_value_modifier>(value);
        return value;
    }

    void player::start_of_turn() {
        if (this != m_game->m_playing && this == m_game->m_first_player) {
            m_game->draw_scenario_card();
        }
        
        m_game->m_playing = this;

        m_bangs_played = 0;
        m_bangs_per_turn = 1;
        m_num_drawn_cards = 0;
        add_player_flags(player_flags::start_of_turn);
        m_declared_suit = card_suit_type::none;

        if (!check_player_flags(player_flags::ghost) && m_hp == 0) {
            if (m_game->has_scenario(scenario_flags::ghosttown)) {
                ++m_num_cards_to_draw;
                for (auto *c : m_characters) {
                    equip_if_enabled(c);
                }
            } else if (m_game->has_scenario(scenario_flags::deadman) && this == m_game->m_first_dead) {
                remove_player_flags(player_flags::dead);
                m_game->add_public_update<game_update_type::player_hp>(id, m_hp = 2);
                m_game->draw_card_to(card_pile_type::player_hand, this);
                m_game->draw_card_to(card_pile_type::player_hand, this);
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
        m_game->queue_event<event_type::pre_turn_start>(this);
        next_predraw_check(nullptr);
    }

    void player::next_predraw_check(card *target_card) {
        if (auto it = m_predraw_checks.find(target_card); it != m_predraw_checks.end()) {
            it->second.resolved = true;
        }
        m_game->queue_delayed_action([this]{
            if (alive() && m_game->m_playing == this && !m_game->m_game_over) {
                if (std::ranges::all_of(m_predraw_checks | std::views::values, &predraw_check::resolved)) {
                    request_drawing();
                } else {
                    m_game->queue_request<request_type::predraw>(this);
                }
            }
        });
    }

    void player::request_drawing() {
        m_game->queue_event<event_type::before_turn_start>(this);
        m_game->queue_event<event_type::on_turn_start>(this);
        m_game->queue_delayed_action([this]{
            m_game->instant_event<event_type::on_request_draw>(this);
            if (m_game->m_requests.empty()) {
                m_game->queue_request<request_type::draw>(this);
            }
        });
    }

    void player::pass_turn() {
        if (m_hand.size() > max_cards_end_of_turn()) {
            m_game->queue_request<request_type::discard_pass>(this);
        } else {
            untap_inactive_cards();

            m_game->instant_event<event_type::on_turn_end>(this);
            if (!check_player_flags(player_flags::extra_turn)) {
                add_player_flags(player_flags::extra_turn);
                m_game->instant_event<event_type::post_turn_end>(this);
            }
            m_game->queue_delayed_action([&]{
                if (m_extra_turns == 0) {
                    if (!check_player_flags(player_flags::ghost) && m_hp == 0 && m_game->has_scenario(scenario_flags::ghosttown)) {
                        --m_num_cards_to_draw;
                        m_game->player_death(nullptr, this);
                    }
                    remove_player_flags(player_flags::extra_turn);
                    m_game->get_next_in_turn(this)->start_of_turn();
                } else {
                    --m_extra_turns;
                    start_of_turn();
                }
            });
        }
    }

    void player::skip_turn() {
        untap_inactive_cards();
        remove_player_flags(player_flags::extra_turn);
        m_game->instant_event<event_type::on_turn_end>(this);
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
            m_game->move_to(m_hand.front(), card_pile_type::discard_pile);
        }
    }

    void player::set_role(player_role role) {
        m_role = role;

        if (role == player_role::sheriff || m_game->m_players.size() <= 3) {
            m_game->add_public_update<game_update_type::player_show_role>(id, m_role, true);
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