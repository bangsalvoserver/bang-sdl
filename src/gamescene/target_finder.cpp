#include "target_finder.h"

#include "game.h"
#include "../manager.h"
#include "../os_api.h"

#include <cassert>

using namespace banggame;
using namespace enums::flag_operators;
using namespace sdl::point_math;

template<typename ... Ts> struct overloaded : Ts ... { using Ts::operator() ...; };
template<typename ... Ts> overloaded(Ts ...) -> overloaded<Ts ...>;

template<game_action_type T, typename ... Ts>
void target_finder::add_action(Ts && ... args) {
    m_game->parent->add_message<banggame::client_message_type::game_action>(enums::enum_tag<T>, std::forward<Ts>(args) ...);
}

void target_finder::set_border_colors() {
    for (auto [pocket, player, card] : m_picking_highlights) {
        switch (pocket) {
        case pocket_type::player_hand:
        case pocket_type::player_table:
        case pocket_type::player_character:
        case pocket_type::selection:
            card->border_color = options.target_finder_can_pick;
            break;
        case pocket_type::main_deck:
            m_game->m_main_deck.border_color = options.target_finder_can_pick;
            break;
        case pocket_type::discard_pile:
            m_game->m_discard_pile.border_color = options.target_finder_can_pick;
            break;
        }
    }
    for (auto *card : m_response_highlights) {
        card->border_color = options.target_finder_can_respond;
    }
    for (auto *card : m_modifiers) {
        card->border_color = options.target_finder_current_card;
    }

    if (m_playing_card) {
        m_playing_card->border_color = options.target_finder_current_card;
    }

    for (auto &[value, is_auto] : m_targets) {
        if (!is_auto) {
            std::visit(overloaded{
                [](target_none) {},
                [](target_other_players) {},
                [](target_player p) {
                    p.player->border_color = options.target_finder_target;
                },
                [&](target_card c) {
                    if (m_selected_cubes.find(c.card) == m_selected_cubes.end()) {
                        c.card->border_color = options.target_finder_target;
                    }
                },
                [](const target_cards &cs) {
                    for (target_card c : cs) {
                        c.card->border_color = options.target_finder_target;
                    }
                }
            }, value);
        }
    }
    for (auto [card, cube] : m_selected_cubes) {
        cube->border_color = options.target_finder_target;
    }
}

bool target_finder::is_card_clickable() const {
    return m_game->m_pending_updates.empty() && m_game->m_animations.empty() && !waiting_confirm();
}

bool target_finder::can_respond_with(card_view *card) const {
    return std::ranges::find(m_response_highlights, card) != m_response_highlights.end();
}

bool target_finder::can_pick(pocket_type pocket, player_view *player, card_view *card) const {
    return std::ranges::find(m_picking_highlights, std::tuple{pocket, player, card}) != m_picking_highlights.end();
}

bool target_finder::can_play_in_turn(player_view *player, card_view *card) const {
    return m_game->m_request_origin_id == 0
        && m_game->m_request_target_id == 0
        && m_game->m_playing_id == m_game->m_player_own_id
        && (!player || player->id == m_game->m_player_own_id);
}

void target_finder::set_response_highlights(const request_status_args &args) {
    clear_status();

    for (int id : args.respond_ids) {
        m_response_highlights.push_back(m_game->find_card(id));
    }

    for (const picking_args &args : args.pick_ids) {
        m_picking_highlights.emplace_back(args.pocket,
            args.player_id ? m_game->find_player(args.player_id) : nullptr,
            args.card_id ? m_game->find_card(args.card_id) : nullptr);
    }
}

void target_finder::clear_status() {
    for (card_view *card : m_response_highlights) {
        card->border_color = {};
    }
    for (auto &[pocket, player, card] : m_picking_highlights) {
        switch (pocket) {
        case pocket_type::player_hand:
        case pocket_type::player_table:
        case pocket_type::player_character:
        case pocket_type::selection:
            card->border_color = {};
            break;
        case pocket_type::main_deck:
            m_game->m_main_deck.border_color = {};
            break;
        case pocket_type::discard_pile:
            m_game->m_discard_pile.border_color = {};
            break;
        }
    }

    m_response_highlights.clear();
    m_picking_highlights.clear();
}

void target_finder::clear_targets() {
    if (m_playing_card) {
        m_playing_card->border_color = {};
    }
    for (card_view *card : m_modifiers) {
        card->border_color = {};
    }
    for (auto &[value, is_auto] : m_targets) {
        if (!is_auto) {
            std::visit(overloaded{
                [](target_none) {},
                [](target_other_players) {},
                [this](target_player p) {
                    p.player->border_color = m_game->m_playing_id == p.player->id ? options.turn_indicator : sdl::color{};
                },
                [](target_card c) {
                    c.card->border_color = {};
                },
                [](const target_cards &cs) {
                    for (target_card c : cs) {
                        c.card->border_color = {};
                    }
                }
            }, value);
        }
    }
    for (auto [card, cube] : m_selected_cubes) {
        cube->border_color = {};
    }
    m_game->m_shop_choice.clear();
    static_cast<target_status &>(*this) = {};
    if (m_forced_card) {
        set_forced_card(m_forced_card);
    }
}

bool target_finder::can_confirm() const {
    if (m_playing_card && !m_equipping) {
        const int neffects = get_current_card_effects().size();
        const int noptionals = get_current_card()->optionals.size();
        return noptionals != 0
            && m_targets.size() >= neffects
            && ((m_targets.size() - neffects) % noptionals == 0);
    }
    return false;
}

void target_finder::on_click_confirm() {
    if (can_confirm()) send_play_card();
}

void target_finder::set_forced_card(card_view *card) {
    m_forced_card = card;
    if (!m_forced_card) {
        return;
    }

    if (card->pocket == &m_game->find_player(m_game->m_player_own_id)->table || card->color == card_color_type::brown) {
        if (can_respond_with(card)) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (card->modifier != card_modifier_type::none) {
            add_modifier(card);
        } else if (verify_modifier(card)) {
            m_playing_card = card;
            handle_auto_targets();
        }
    } else {
        if (card->self_equippable()) {
            m_playing_card = card;
            m_targets.emplace_back(target_player{m_game->find_player(m_game->m_playing_id)}, true);
            send_play_card();
        } else {
            m_playing_card = card;
            m_equipping = true;
        }
    }
}

void target_finder::send_pick_card(pocket_type pocket, player_view *player, card_view *card) {
    add_action<game_action_type::pick_card>(pocket, player ? player->id : 0, card ? card->id : 0);
    m_waiting_confirm = true;
}

void target_finder::on_click_discard_pile() {
    if (can_pick(pocket_type::discard_pile, nullptr, nullptr)) {
        send_pick_card(pocket_type::discard_pile);
    }
}

void target_finder::on_click_main_deck() {
    if (can_pick(pocket_type::main_deck, nullptr, nullptr)) {
        send_pick_card(pocket_type::main_deck);
    }
}

void target_finder::on_click_selection_card(card_view *card) {
    if (can_pick(pocket_type::selection, nullptr, card)) {
        send_pick_card(pocket_type::selection, nullptr, card);
    }
}

void target_finder::on_click_shop_card(card_view *card) {
    if (!m_playing_card
        && m_game->m_playing_id == m_game->m_player_own_id
        && m_game->m_request_origin_id == 0
        && m_game->m_request_target_id == 0)
    {
        int cost = card->buy_cost();
        if (std::ranges::find(m_modifiers, card_modifier_type::discount, &card_view::modifier) != m_modifiers.end()) {
            --cost;
        }
        if (m_game->find_player(m_game->m_player_own_id)->gold >= cost) {
            if (card->color == card_color_type::black) {
                if (verify_modifier(card)) {
                    if (card->self_equippable()) {
                        m_playing_card = card;
                        m_targets.emplace_back(target_player{m_game->find_player(m_game->m_playing_id)}, true);
                        send_play_card();
                    } else {
                        m_playing_card = card;
                        m_equipping = true;
                    }
                }
            } else if (card->modifier != card_modifier_type::none) {
                add_modifier(card);
            } else if (verify_modifier(card)) {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    }
}

void target_finder::on_click_table_card(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card) && !card->inactive) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (can_pick(pocket_type::player_table, player, card)) {
            send_pick_card(pocket_type::player_table, player, card);
        } else if (can_play_in_turn(player, card) && !card->inactive) {
            if (card->modifier != card_modifier_type::none) {
                add_modifier(card);
            } else if (verify_modifier(card)) {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    } else {
        add_card_target(target_card{player, card});
    }
}

void target_finder::on_click_hand_card(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (can_pick(pocket_type::player_hand, player, card)) {
            send_pick_card(pocket_type::player_hand, player, card);
        } else if (can_play_in_turn(player, card)) {
            if (card->color == card_color_type::brown) {
                if (card->modifier != card_modifier_type::none) {
                    add_modifier(card);
                } else if (verify_modifier(card)) {
                    m_playing_card = card;
                    handle_auto_targets();
                }
            } else if (m_modifiers.empty()) {
                if (card->self_equippable()) {
                    m_targets.emplace_back(target_player{player}, true);
                    m_playing_card = card;
                    send_play_card();
                } else {
                    m_playing_card = card;
                    m_equipping = true;
                }
            } else if (verify_modifier(card)) {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    } else {
        add_card_target(target_card{player, card});
    }
}

void target_finder::on_click_character(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (can_pick(pocket_type::player_character, player, card)) {
            send_pick_card(pocket_type::player_character, player, card);
        } else if (can_play_in_turn(player, card)) {
            if (card->modifier != card_modifier_type::none) {
                add_modifier(card);
            } else if (!card->effects.empty()) {
                m_playing_card = card;
                handle_auto_targets();
            }
        }
    } else {
        add_character_target(target_card{player, card});
    }
}

void target_finder::on_click_scenario_card(card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            m_playing_card = card;
            m_response = true;
            handle_auto_targets();
        } else if (can_play_in_turn(nullptr, card)) {
            m_playing_card = card;
            handle_auto_targets();
        }
    }
}

bool target_finder::on_click_player(player_view *player) {
    auto verify_target = [&](const auto &args) {
        if constexpr (requires { args.target; }) {
            if (args.target != play_card_target_type::player) {
                return false;
            }
        }
        if (auto error = verify_player_target(args.player_filter, player))  {
            m_game->parent->add_chat_message(message_type::error, *error);
            os_api::play_bell();
            return false;
        } else {
            return true;
        }
    };

    if (m_playing_card) {
        if (m_equipping) {
            if (verify_target(m_playing_card->equips[m_targets.size()])) {
                m_targets.emplace_back(target_player{player});
                send_play_card();
                return true;
            }
        } else if (std::ranges::find(m_targets, target_variant_base{target_player{player}}, &target_variant::value) == m_targets.end()
            && verify_target(get_effect_holder(get_target_index())))
        {
            m_targets.emplace_back(target_player{player});
            handle_auto_targets();
            return true;
        }
    }
    return false;
}

void target_finder::add_modifier(card_view *card) {
    if (std::ranges::find(m_modifiers, card) == m_modifiers.end()) {
        switch (card->modifier) {
        case card_modifier_type::bangmod:
        case card_modifier_type::bandolier:
            if (std::ranges::all_of(m_modifiers, [](const card_view *c) {
                return c->modifier == card_modifier_type::bangmod
                    || c->modifier == card_modifier_type::bandolier
                    || c->modifier == card_modifier_type::belltower;
            })) {
                m_modifiers.push_back(card);
            }
            break;
        case card_modifier_type::leevankliff:
            if (m_modifiers.empty() && m_last_played_card) {
                m_modifiers.push_back(card);
            }
            break;
        case card_modifier_type::discount:
        case card_modifier_type::shopchoice:
            if (std::ranges::all_of(m_modifiers, [](const card_view *c) {
                return c->modifier == card_modifier_type::discount
                    || c->modifier == card_modifier_type::shopchoice
                    || c->modifier == card_modifier_type::belltower;
            })) {
                if (card->modifier == card_modifier_type::shopchoice) {
                    for (card_view *c : m_game->m_hidden_deck) {
                        if (c->get_tag_value(tag_type::shopchoice) == card->get_tag_value(tag_type::shopchoice)) {
                            m_game->m_shop_choice.add_card(c);
                        }
                    }
                    for (card_view *c : m_game->m_shop_choice) {
                        c->set_pos(m_game->m_shop_choice.get_pos() + m_game->m_shop_choice.get_offset(c));
                    }
                }
                m_modifiers.push_back(card);
            }
            break;
        case card_modifier_type::belltower:
            if (m_modifiers.empty() || m_modifiers.front()->modifier != card_modifier_type::leevankliff) {
                m_modifiers.push_back(card);
            }
            break;
        default:
            break;
        }
    }
}

bool target_finder::is_bangcard(card_view *card) {
    return (m_game->has_player_flags(player_flags::treat_missed_as_bang)
            && card->has_tag(tag_type::missedcard))
        || card->has_tag(tag_type::bangcard);
}

bool target_finder::verify_modifier(card_view *card) {
    return std::ranges::all_of(m_modifiers, [&](card_view *c) {
        switch (c->modifier) {
        case card_modifier_type::bangmod:
        case card_modifier_type::bandolier:
            return is_bangcard(card) || card->has_tag(tag_type::bangproxy);
        case card_modifier_type::leevankliff:
            return is_bangcard(card) && m_last_played_card;
        case card_modifier_type::discount:
            return card->expansion == card_expansion_type::goldrush;
        case card_modifier_type::shopchoice:
            return std::ranges::find(m_game->m_shop_choice, card) != m_game->m_shop_choice.end();
        case card_modifier_type::belltower: {
            player_view *p = m_game->find_player(m_game->m_player_own_id);
            if (card->pocket == &p->hand) {
                return card->color == card_color_type::brown;
            } else if (card->pocket == &p->table) {
                return !card->effects.empty();
            } else {
                return card->color != card_color_type::black;
            }
        }
        default:
            return false;
        }
    });
}

const card_view *target_finder::get_current_card() const {
    assert(!m_equipping);

    if (m_last_played_card && !m_modifiers.empty() && m_modifiers.front()->modifier == card_modifier_type::leevankliff) {
        return m_last_played_card;
    } else {
        return m_playing_card;
    }
}

const effect_list &target_finder::get_current_card_effects() const {
    if (m_response) {
        return get_current_card()->responses;
    } else {
        return get_current_card()->effects;
    }
}

const effect_holder &target_finder::get_effect_holder(int index) {
    auto &effects = get_current_card_effects();
    if (index < effects.size()) {
        return effects[index];
    }

    auto &optionals = get_current_card()->optionals;
    return optionals[(index - effects.size()) % optionals.size()];
}

int target_finder::num_targets_for(const effect_holder &data) {
    if (data.target == play_card_target_type::cards_other_players) {
        return std::ranges::count_if(m_game->m_players, [&](const player_view &p) {
            if (p.dead || p.id == m_game->m_player_own_id) return false;
            if (p.table.empty() && p.hand.empty()) return false;
            return true;
        });
    } else {
        return 1;
    }
};

int target_finder::get_target_index() {
    if (m_targets.empty()) return 0;
    int index = m_targets.size() - 1;
    size_t size = std::visit(overloaded{
        [](const auto &) -> size_t { return 1; },
        []<typename T>(const std::vector<T> &value) { return value.size(); }
    }, m_targets[index].value);
    index += size >= num_targets_for(get_effect_holder(index));
    return index;
}

int target_finder::calc_distance(player_view *from, player_view *to) {
    if (std::ranges::find(m_modifiers, card_modifier_type::belltower, &card_view::modifier) != m_modifiers.end()) {
        return 1;
    }
    
    auto get_next_player = [&](player_view *p) {
        auto it = m_game->m_players.find(p->id);
        do {
            if (++it == m_game->m_players.end()) it = m_game->m_players.begin();
        } while(it->dead);
        return &*it;
    };

    if (from == to) return 0;
    if (from->has_player_flags(player_flags::disable_player_distances)) return to->m_distance_mod;
    int d1=0, d2=0;
    for (player_view *counter = from; counter != to; counter = get_next_player(counter), ++d1);
    for (player_view *counter = to; counter != from; counter = get_next_player(counter), ++d2);
    return std::min(d1, d2) + to->m_distance_mod;
}

void target_finder::handle_auto_targets() {
    using namespace enums::flag_operators;

    auto *self_player = m_game->find_player(m_game->m_player_own_id);
    auto do_handle_target = [&](const effect_holder &data) {
        switch (data.target) {
        case play_card_target_type::none:
            m_targets.emplace_back(target_none{}, true);
            return true;
        case play_card_target_type::player:
            if (data.player_filter == target_player_filter::self) {
                m_targets.emplace_back(target_player{self_player}, true);
                return true;
            }
            break;
        case play_card_target_type::other_players:
            m_targets.emplace_back(target_other_players{}, true);
            return true;
        }
        return false;
    };

    auto &effects = get_current_card_effects();
    auto &optionals = get_current_card()->optionals;
    auto repeatable = get_current_card()->get_tag_value(tag_type::repeatable);
    if (effects.empty()) {
        clear_targets();
    } else {
        auto effect_it = effects.begin() + m_targets.size();
        auto target_end = effects.end();
        if (effect_it >= target_end && !optionals.empty()) {
            int diff = m_targets.size() - effects.size();
            effect_it = optionals.begin() + (repeatable ? diff % optionals.size() : diff);
            target_end = optionals.end();
        }
        for(;;++effect_it) {
            if (effect_it == target_end) {
                if (optionals.empty() || (target_end == optionals.end()
                    && (!repeatable || (*repeatable > 0
                        && m_targets.size() - effects.size() == optionals.size() * *repeatable))))
                {
                    send_play_card();
                    break;
                }
                effect_it = optionals.begin();
                target_end = optionals.end();
            }
            if (!do_handle_target(*effect_it)) break;
        }
    }
}

std::optional<std::string> target_finder::verify_player_target(target_player_filter filter, player_view *target_player) {
    if (bool(filter & target_player_filter::dead) != target_player->dead)
        return _("ERROR_TARGET_DEAD");

    if (bool(filter & target_player_filter::self) && target_player->id != m_game->m_player_own_id)
        return _("ERROR_TARGET_NOT_SELF");

    if (bool(filter & target_player_filter::notself) && target_player->id == m_game->m_player_own_id)
        return _("ERROR_TARGET_SELF");

    if (bool(filter & target_player_filter::notsheriff) && target_player->m_role->role == player_role::sheriff)
        return _("ERROR_TARGET_SHERIFF");

    if (bool(filter & (target_player_filter::reachable | target_player_filter::range_1 | target_player_filter::range_2))) {
        player_view *own_player = m_game->find_player(m_game->m_player_own_id);
        int distance = own_player->m_range_mod;
        if (bool(filter & target_player_filter::reachable)) {
            distance += own_player->m_weapon_range;
        } else if (bool(filter & target_player_filter::range_1)) {
            ++distance;
        } else if (bool(filter & target_player_filter::range_2)) {
            distance += 2;
        }
        if (calc_distance(own_player, target_player) > distance) {
            return _("ERROR_TARGET_NOT_IN_RANGE");
        }
    }

    return std::nullopt;
}

std::optional<std::string> target_finder::verify_card_target(const effect_holder &args, target_card target) {
    if (auto error = verify_player_target(args.player_filter, target.player))
        return error;

    if (bool(args.card_filter & target_card_filter::black) != (target.card->color == card_color_type::black))
        return _("ERROR_TARGET_BLACK_CARD");
    
    if (bool(args.card_filter & target_card_filter::table) && target.card->pocket != &target.player->table)
        return _("ERROR_TARGET_NOT_TABLE_CARD");

    if (bool(args.card_filter & target_card_filter::hand) && target.card->pocket != &target.player->hand)
        return _("ERROR_TARGET_NOT_HAND_CARD");
    
    if (bool(args.card_filter & target_card_filter::blue) && target.card->color != card_color_type::blue)
        return _("ERROR_TARGET_NOT_BLUE_CARD");

    if (bool(args.card_filter & target_card_filter::clubs) && target.card->sign.suit != card_suit::clubs)
        return _("ERROR_TARGET_NOT_CLUBS");
    
    if (bool(args.card_filter & target_card_filter::bang) && !is_bangcard(target.card))
        return _("ERROR_TARGET_NOT_BANG");
    
    if (bool(args.card_filter & target_card_filter::missed) && !target.card->has_tag(tag_type::missedcard))
        return _("ERROR_TARGET_NOT_MISSED");
    
    if (bool(args.card_filter & target_card_filter::beer) && !target.card->has_tag(tag_type::beer))
        return _("ERROR_TARGET_NOT_BEER");
    
    if (bool(args.card_filter & target_card_filter::bronco) && !target.card->has_tag(tag_type::bronco))
        return _("ERROR_TARGET_NOT_BRONCO");
    
    if (bool(args.card_filter & (target_card_filter::cube_slot | target_card_filter::cube_slot_card))
        && (target.card != target.player ->m_characters.front() && target.card->color != card_color_type::orange))
        return _("ERROR_TARGET_NOT_CUBE_SLOT");

    return std::nullopt;
}

void target_finder::add_card_target(target_card target) {
    if (m_equipping) return;

    int index = get_target_index();
    auto cur_target = get_effect_holder(index);

    if (cur_target.target == play_card_target_type::card || cur_target.target == play_card_target_type::cards_other_players) {
        if (auto error = verify_card_target(cur_target, target)) {
            m_game->parent->add_chat_message(message_type::error, *error);
            os_api::play_bell();
            return;
        }
    } else {
        return;
    }
    
    if (bool(cur_target.card_filter & target_card_filter::cube_slot)) {
        int ncubes = m_selected_cubes.count(target.card);
        if (ncubes < target.card->cubes.size()) {
            m_targets.emplace_back(target);
            m_selected_cubes.emplace(target.card, (target.card->cubes.rbegin() + ncubes)->get());
            handle_auto_targets();
        }
    } else if (target.card != m_playing_card && std::ranges::find(m_modifiers, target.card) == m_modifiers.end()) {
        if (cur_target.target == play_card_target_type::cards_other_players) {
            if (index >= m_targets.size()) {
                m_targets.emplace_back(target_cards{});
            }
            auto &vec = std::get<target_cards>(m_targets.back().value);
            if (target.player->id != m_game->m_player_own_id
                && std::ranges::find(vec, target.player, &target_card::player) == vec.end())
            {
                vec.push_back(target);
                if (vec.size() == num_targets_for(cur_target)) {
                    handle_auto_targets();
                }
            }
        } else if (bool(cur_target.card_filter & target_card_filter::can_repeat)
            || std::ranges::find(m_targets, target_variant_base{target}, &target_variant::value) == m_targets.end())
        {
            m_targets.emplace_back(target);
            handle_auto_targets();
        }
    }
}

void target_finder::add_character_target(target_card target) {
    if (m_equipping) return;

    const auto &effect = get_effect_holder(get_target_index());
    if (!bool(effect.card_filter & (target_card_filter::cube_slot | target_card_filter::cube_slot_card))) return;
    
    if (std::ranges::find(target.player->m_characters, target.card) != target.player->m_characters.end()) {
        target.card = target.player->m_characters.front();
    } else {
        return;
    }

    if (auto error = verify_card_target(effect, target)) {
        m_game->parent->add_chat_message(message_type::error, *error);
        os_api::play_bell();
        return;
    }

    if(bool(effect.card_filter & target_card_filter::cube_slot_card)) {
        m_targets.emplace_back(target);
        handle_auto_targets();
    } else {
        int ncubes = m_selected_cubes.count(target.card);
        if (ncubes < target.card->cubes.size()) {
            m_targets.emplace_back(target);
            m_selected_cubes.emplace(target.card, (target.card->cubes.rbegin() + ncubes)->get());
            handle_auto_targets();
        }
    }
}

void target_finder::send_play_card() {
    play_card_args ret;
    ret.card_id = m_playing_card ? m_playing_card->id : 0;
    for (card_view *card : m_modifiers) {
        ret.modifier_ids.push_back(card->id);
    }

    for (const auto &target : m_targets) {
        std::visit(overloaded{
            [&](target_none) {
                ret.targets.emplace_back(enums::enum_tag<play_card_target_type::none>);
            },
            [&](target_player p) {
                ret.targets.emplace_back(enums::enum_tag<play_card_target_type::player>, p.player->id);
            },
            [&](target_other_players) {
                ret.targets.emplace_back(enums::enum_tag<play_card_target_type::other_players>);
            },
            [&](target_card c) {
                ret.targets.emplace_back(enums::enum_tag<play_card_target_type::card>, c.card->id);
            },
            [&](const target_cards &cs) {
                std::vector<int> ids;
                for (auto [player, card] : cs) {
                    ids.push_back(card->id);
                }
                ret.targets.emplace_back(enums::enum_tag<play_card_target_type::cards_other_players>, std::move(ids));
            }
        }, target.value);
    }
    if (m_response) {
        add_action<game_action_type::respond_card>(std::move(ret));
    } else {
        add_action<game_action_type::play_card>(std::move(ret));
    }

    m_waiting_confirm = true;
}

void target_finder::send_prompt_response(bool response) {
    add_action<game_action_type::prompt_respond>(response);
}

void target_finder::confirm_play(bool valid) {
    if (valid) {
        m_game->m_ui.close_message_box();
        m_forced_card = nullptr;
    }
    m_waiting_confirm = false;
    clear_targets();
}