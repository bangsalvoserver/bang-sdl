#include "target_finder.h"

#include "game.h"
#include "../manager.h"
#include "../os_api.h"

#include <cassert>
#include <numeric>

using namespace banggame;
using namespace enums::flag_operators;
using namespace sdl::point_math;

template<typename ... Ts> struct overloaded : Ts ... { using Ts::operator() ...; };
template<typename ... Ts> overloaded(Ts ...) -> overloaded<Ts ...>;

template<std::ranges::input_range R, typename T, typename Proj = std::identity>
requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<R>, Proj>, const T *>
constexpr bool ranges_contains(R &&r, const T &value, Proj proj = {}) {
    return std::ranges::find(r, value, proj) != std::ranges::end(r);
}

void target_finder::set_playing_card(card_view *card, bool is_response) {
    m_response = is_response && !bool(m_request_flags & effect_flags::force_play);
    m_playing_card = card;
    m_target_borders.add(card->border_color, options.target_finder_current_card);
}

template<game_action_type T>
void target_finder::add_action(auto && ... args) {
    m_game->parent->add_message<banggame::client_message_type::game_action>(enums::enum_tag<T>, FWD(args) ...);
}

bool target_finder::is_card_clickable() const {
    return m_game->m_pending_updates.empty() && m_game->m_animations.empty() && !waiting_confirm();
}

bool target_finder::can_respond_with(card_view *card) const {
    return ranges_contains(m_response_highlights, card);
}

bool target_finder::can_play_in_turn(player_view *player, card_view *card) const {
    if (bool(m_request_flags & effect_flags::force_play)) {
        return can_respond_with(card);
    } else {
        return !m_game->m_request_origin && !m_game->m_request_target
            && m_game->m_playing == m_game->m_player_self
            && (!player || player == m_game->m_player_self);
    }
}

void target_finder::set_response_highlights(const request_status_args &args) {
    if (m_game->m_player_self) {
        add_action<game_action_type::request_confirm>();
    }
    clear_status();

    for (const picking_args &args : args.pick_ids) {
        const auto &[pocket, player, card] = m_picking_highlights.emplace_back(
            args.pocket,
            args.player_id ? m_game->find_player(args.player_id) : nullptr,
            args.card_id ? m_game->find_card(args.card_id) : nullptr);
    
        switch (pocket) {
        case pocket_type::player_hand:
        case pocket_type::player_table:
        case pocket_type::player_character:
        case pocket_type::selection:
            m_response_borders.add(card->border_color, options.target_finder_can_pick);
            break;
        case pocket_type::main_deck:
            m_response_borders.add(m_game->m_main_deck.border_color, options.target_finder_can_pick);
            break;
        case pocket_type::discard_pile:
            m_response_borders.add(m_game->m_discard_pile.border_color, options.target_finder_can_pick);
            break;
        }
    }

    for (int id : args.respond_ids) {
        card_view *card = m_response_highlights.emplace_back(m_game->find_card(id));
        m_response_borders.add(card->border_color, options.target_finder_can_respond);
    }

    m_request_flags = args.flags;
    handle_auto_respond();
}

void target_finder::clear_status() {
    m_request_flags = {};
    m_response_highlights.clear();
    m_picking_highlights.clear();
    clear_targets();
    m_response_borders.clear();
}

void target_finder::clear_targets() {
    m_game->m_shop_choice.clear();
    static_cast<target_status &>(*this) = {};
    handle_auto_respond();
}

void target_finder::handle_auto_respond() {
    if (!m_playing_card && !waiting_confirm() && bool(m_request_flags & effect_flags::auto_respond) && m_response_highlights.size() == 1 && m_picking_highlights.empty()) {
        card_view *card = m_response_highlights.front();
        if (card->color == card_color_type::black) {
            set_playing_card(card);
            m_equipping = true;
        } else if (card->modifier != card_modifier_type::none) {
            add_modifier(card);
        } else if (verify_modifier(card)) {
            set_playing_card(card, true);
            handle_auto_targets();
        }
    }
}

bool target_finder::can_confirm() const {
    if (m_playing_card && !m_equipping) {
        const size_t neffects = get_current_card_effects().size();
        const size_t noptionals = get_current_card()->optionals.size();
        return noptionals != 0
            && m_targets.size() >= neffects
            && ((m_targets.size() - neffects) % noptionals == 0);
    }
    return false;
}

void target_finder::on_click_confirm() {
    if (can_confirm()) send_play_card();
}

bool target_finder::send_pick_card(pocket_type pocket, player_view *player, card_view *card) {
    if (ranges_contains(m_picking_highlights, std::tuple{pocket, player, card})) {
        add_action<game_action_type::pick_card>(pocket, player ? player->id : 0, card ? card->id : 0);
        m_waiting_confirm = true;
        return true;
    }
    return false;
}

void target_finder::on_click_discard_pile() {
    send_pick_card(pocket_type::discard_pile);
}

void target_finder::on_click_main_deck() {
    send_pick_card(pocket_type::main_deck);
}

void target_finder::on_click_selection_card(card_view *card) {
    send_pick_card(pocket_type::selection, nullptr, card);
}

void target_finder::on_click_shop_card(card_view *card) {
    const bool is_forced_card = bool(m_request_flags & effect_flags::force_play);
    if (!m_playing_card && m_game->m_playing == m_game->m_player_self
        && (is_forced_card || (!m_game->m_request_origin && !m_game->m_request_target)))
    {
        if (!is_forced_card) {
            int cost = card->buy_cost();
            if (ranges_contains(m_modifiers, card_modifier_type::discount, &card_view::modifier)) {
                --cost;
            }
            if (m_game->m_player_self->gold < cost) return;
        }
        if (card->color == card_color_type::black) {
            if (verify_modifier(card)) {
                if (card->self_equippable()) {
                    set_playing_card(card);
                    send_play_card();
                } else {
                    set_playing_card(card);
                    m_equipping = true;
                }
            }
        } else if (card->modifier != card_modifier_type::none) {
            add_modifier(card);
        } else if (verify_modifier(card)) {
            set_playing_card(card);
            handle_auto_targets();
        }
    }
}

void target_finder::on_click_table_card(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card) && !card->inactive) {
            set_playing_card(card, true);
            handle_auto_targets();
        } else if (!send_pick_card(pocket_type::player_table, player, card)
                && can_play_in_turn(player, card) && !card->inactive) {
            if (card->modifier != card_modifier_type::none) {
                add_modifier(card);
            } else if (verify_modifier(card)) {
                set_playing_card(card);
                handle_auto_targets();
            }
        }
    } else {
        add_card_target(player, card);
    }
}

void target_finder::on_click_character(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            set_playing_card(card, true);
            handle_auto_targets();
        } else if (!send_pick_card(pocket_type::player_character, player, card)
                && can_play_in_turn(player, card)) {
            if (card->modifier != card_modifier_type::none) {
                add_modifier(card);
            } else if (!card->effects.empty()) {
                set_playing_card(card);
                handle_auto_targets();
            }
        }
    } else {
        add_card_target(player, player->m_characters.front());
    }
}

void target_finder::on_click_hand_card(player_view *player, card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            set_playing_card(card, true);
            handle_auto_targets();
        } else if (!send_pick_card(pocket_type::player_hand, player, card)
                && can_play_in_turn(player, card)) {
            if (card->color == card_color_type::brown) {
                if (card->modifier != card_modifier_type::none) {
                    add_modifier(card);
                } else if (verify_modifier(card)) {
                    set_playing_card(card);
                    handle_auto_targets();
                }
            } else if (m_modifiers.empty()) {
                if (card->self_equippable()) {
                    set_playing_card(card);
                    send_play_card();
                } else {
                    set_playing_card(card);
                    m_equipping = true;
                }
            } else if (verify_modifier(card)) {
                set_playing_card(card);
                handle_auto_targets();
            }
        }
    } else {
        add_card_target(player, card);
    }
}

void target_finder::on_click_scenario_card(card_view *card) {
    if (!m_playing_card) {
        if (can_respond_with(card)) {
            set_playing_card(card, true);
            handle_auto_targets();
        } else if (can_play_in_turn(nullptr, card)) {
            set_playing_card(card);
            handle_auto_targets();
        }
    }
}

bool target_finder::on_click_player(player_view *player) {
    auto verify_filter = [&](target_player_filter filter) {
        if (auto error = verify_player_target(filter, player))  {
            m_game->parent->add_chat_message(message_type::error, *error);
            os_api::play_bell();
            return false;
        }
        return true;
    };

    if (!m_playing_card) {
        return false;
    } else if (m_equipping) {
        if (verify_filter(m_playing_card->equip_target)) {
            m_target_borders.add(player->border_color, options.target_finder_target);
            m_targets.emplace_back(player);
            send_play_card();
            return true;
        }
    } else if (const auto &args = get_effect_holder(get_target_index()); !verify_filter(args.player_filter)) {
        return true;
    } else if (args.target == target_type::player || args.target == target_type::conditional_player) {
        if (ranges_contains(possible_player_targets(args.player_filter), player)) {
            m_target_borders.add(player->border_color, options.target_finder_target);
            if (args.target == target_type::player) {
                m_targets.emplace_back(player);
            } else {
                m_targets.emplace_back(nullable(player));
            }
            handle_auto_targets();
        }
        return true;
    }
    return false;
}

void target_finder::add_modifier(card_view *card) {
    if (!ranges_contains(m_modifiers, card)) {
        switch (card->modifier) {
        case card_modifier_type::bangmod:
        case card_modifier_type::bandolier:
            if (std::ranges::all_of(m_modifiers, [](const card_view *c) {
                return c->modifier == card_modifier_type::bangmod
                    || c->modifier == card_modifier_type::bandolier
                    || c->modifier == card_modifier_type::belltower;
            })) {
                m_modifiers.push_back(card);
                m_target_borders.add(card->border_color, options.target_finder_current_card);
            }
            break;
        case card_modifier_type::leevankliff:
            if (m_modifiers.empty() && m_last_played_card) {
                m_modifiers.push_back(card);
                m_target_borders.add(card->border_color, options.target_finder_current_card);
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
                m_target_borders.add(card->border_color, options.target_finder_current_card);
            }
            break;
        case card_modifier_type::belltower:
            if (m_modifiers.empty() || m_modifiers.front()->modifier != card_modifier_type::leevankliff) {
                m_modifiers.push_back(card);
                m_target_borders.add(card->border_color, options.target_finder_current_card);
            }
            break;
        default:
            break;
        }
    }
}

bool target_finder::is_bangcard(card_view *card) {
    return (m_game->m_player_self->has_player_flags(player_flags::treat_missed_as_bang)
            && card->has_tag(tag_type::missedcard))
        || card->has_tag(tag_type::bangcard);
}

bool target_finder::verify_modifier(card_view *card) {
    return std::ranges::all_of(m_modifiers, [&](card_view *c) {
        switch (c->modifier) {
        case card_modifier_type::bangmod:
        case card_modifier_type::bandolier:
            if (card->pocket == &m_game->m_playing->hand) {
                return is_bangcard(card);
            } else {
                return card->has_tag(tag_type::play_as_bang);
            }
        case card_modifier_type::leevankliff:
            return is_bangcard(card) && m_last_played_card;
        case card_modifier_type::discount:
            return card->expansion == card_expansion_type::goldrush;
        case card_modifier_type::shopchoice:
            return ranges_contains(m_game->m_shop_choice, card);
        case card_modifier_type::belltower:
            if (card->pocket == &m_game->m_player_self->hand) {
                return card->color == card_color_type::brown;
            } else if (card->pocket == &m_game->m_player_self->table) {
                return !card->effects.empty();
            } else {
                return card->color != card_color_type::black;
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

int target_finder::count_selected_cubes(card_view *card) {
    size_t sum = 0;
    if (ranges_contains(m_modifiers, card) || card == m_playing_card) {
        for (const auto &e : card->effects) {
            if (e.type == effect_type::pay_cube) {
                sum += std::clamp<size_t>(e.effect_value, 1, 4);
            }
        }
    }
    for (const auto &t : m_targets) {
        if (auto *val = std::get_if<std::vector<card_cube_pair>>(&t)) {
            sum += std::ranges::count(*val, card, &card_cube_pair::first);
        }
    }
    return static_cast<int>(sum);
}

int target_finder::get_target_index() {
    if (m_targets.empty()) {
        return 0;
    }
    return static_cast<int>(m_targets.size() - enums::visit(overloaded{
        [](const auto &) {
            return false;
        },
        []<typename T>(const std::vector<T> &value) {
            return value.size() != value.capacity();
        }
    }, m_targets.back()));
}

int target_finder::calc_distance(player_view *from, player_view *to) {
    if (from == to) return 0;
    if (bool(m_game->m_game_flags & game_flags::disable_player_distances)) return to->m_distance_mod;

    if (ranges_contains(m_modifiers, card_modifier_type::belltower, &card_view::modifier)) {
        return 1;
    }

    struct player_view_iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = int;
        using value_type = player_view;
        using pointer = player_view *;
        using reference = player_view &;

        util::id_map<banggame::player_view> *list;
        util::id_map<banggame::player_view>::iterator m_it;

        player_view_iterator() = default;
        
        player_view_iterator(game_scene *game, player_view *p)
            : list(&game->m_players), m_it(list->find(p->id)) {}
        
        player_view_iterator &operator ++() {
            do {
                ++m_it;
                if (m_it == list->end()) m_it = list->begin();
            } while (!m_it->alive());
            return *this;
        }

        player_view_iterator operator ++(int) { auto copy = *this; ++*this; return copy; }

        player_view_iterator &operator --() {
            do {
                if (m_it == list->begin()) m_it = list->end();
                --m_it;
            } while (!m_it->alive());
            return *this;
        }

        player_view_iterator operator --(int) { auto copy = *this; --*this; return copy; }

        bool operator == (const player_view_iterator &) const = default;
    };

    if (to->alive()) {
        return std::min(
            std::distance(player_view_iterator(m_game, from), player_view_iterator(m_game, to)),
            std::distance(player_view_iterator(m_game, to), player_view_iterator(m_game, from))
        ) + to->m_distance_mod;
    } else {
        return std::min(
            std::distance(player_view_iterator(m_game, from), std::prev(player_view_iterator(m_game, to))),
            std::distance(std::next(player_view_iterator(m_game, to)), player_view_iterator(m_game, from))
        ) + 1 + to->m_distance_mod;
    }
}

void target_finder::handle_auto_targets() {
    using namespace enums::flag_operators;

    auto *current_card = get_current_card();

    auto do_handle_target = [&](const effect_holder &data) {
        switch (data.target) {
        case target_type::none:
            m_targets.emplace_back(enums::enum_tag<target_type::none>);
            return true;
        case target_type::conditional_player:
            if (possible_player_targets(data.player_filter).empty()) {
                m_targets.emplace_back(enums::enum_tag<target_type::conditional_player>);
                return true;
            }
            break;
        case target_type::extra_card:
            if (current_card == m_last_played_card) {
                m_targets.emplace_back(enums::enum_tag<target_type::extra_card>);
                return true;
            }
            break;
        case target_type::other_players:
            m_targets.emplace_back(enums::enum_tag<target_type::other_players>);
            return true;
        case target_type::all_players:
            m_targets.emplace_back(enums::enum_tag<target_type::all_players>);
            return true;
        }
        return false;
    };

    auto &effects = get_current_card_effects();
    auto &optionals = current_card->optionals;
    auto repeatable = current_card->get_tag_value(tag_type::repeatable);
    
    auto is_auto_confirmable = [&]{
        if (can_confirm()) {
            if (current_card->has_tag(tag_type::auto_confirm)) {
                return std::ranges::any_of(optionals, [&](const effect_holder &holder) {
                    return holder.target == target_type::player
                        && possible_player_targets(holder.player_filter).empty();
                });
            } else if (current_card->has_tag(tag_type::auto_confirm_red_ringo)) {
                return current_card->cubes.size() <= 1
                    || std::transform_reduce(m_game->m_playing->table.begin(), m_game->m_playing->table.end(), 0, std::plus(), [](const card_view *card) {
                        return static_cast<int>(4 - card->cubes.size());
                    }) <= 1;
            }
        } else if (repeatable && *repeatable) {
            return m_targets.size() - effects.size() == optionals.size() * *repeatable;
        }
        return false;
    };

    if (effects.empty()) {
        clear_targets();
    } else if (is_auto_confirmable()) {
        send_play_card();
    } else {
        auto effect_it = effects.data();
        auto target_end = effects.data() + effects.size();
        if (m_targets.size() >= effects.size() && !optionals.empty()) {
            size_t diff = m_targets.size() - effects.size();
            effect_it = optionals.data() + (repeatable ? diff % optionals.size() : diff);
            target_end = optionals.data() + optionals.size();
        } else {
            effect_it += m_targets.size();
        }
        while (true) {
            if (effect_it == target_end) {
                if (optionals.empty() || (target_end == (optionals.data() + optionals.size()) && !repeatable)) {
                    send_play_card();
                    break;
                }
                effect_it = optionals.data();
                target_end = optionals.data() + optionals.size();
            }
            if (!do_handle_target(*effect_it)) break;
            ++effect_it;
        }
    }
}

std::optional<std::string> target_finder::verify_player_target(target_player_filter filter, player_view *target_player) {
    if (bool(filter & target_player_filter::dead)) {
        if (target_player->hp > 0) return _("ERROR_TARGET_NOT_DEAD");
    } else if (!target_player->has_player_flags(player_flags::targetable) && !target_player->alive()) {
        return _("ERROR_TARGET_DEAD");
    }

    if (bool(filter & target_player_filter::self) && target_player != m_game->m_player_self)
        return _("ERROR_TARGET_NOT_SELF");

    if (bool(filter & target_player_filter::notself) && target_player == m_game->m_player_self)
        return _("ERROR_TARGET_SELF");

    if (bool(filter & target_player_filter::notsheriff) && target_player->m_role->role == player_role::sheriff)
        return _("ERROR_TARGET_SHERIFF");

    if (bool(filter & (target_player_filter::reachable | target_player_filter::range_1 | target_player_filter::range_2))) {
        int distance = m_game->m_player_self->m_range_mod;
        if (bool(filter & target_player_filter::reachable)) {
            distance += m_game->m_player_self->m_weapon_range;
        } else if (bool(filter & target_player_filter::range_1)) {
            ++distance;
        } else if (bool(filter & target_player_filter::range_2)) {
            distance += 2;
        }
        if (calc_distance(m_game->m_player_self, target_player) > distance) {
            return _("ERROR_TARGET_NOT_IN_RANGE");
        }
    }

    return std::nullopt;
}

std::optional<std::string> target_finder::verify_card_target(const effect_holder &args, player_view *player, card_view *card) {
    if (!get_current_card()->has_tag(tag_type::can_target_self)
        && card == m_playing_card || ranges_contains(m_modifiers, card))
        return _("ERROR_TARGET_PLAYING_CARD");

    if (bool(args.card_filter & target_card_filter::cube_slot)) {
        if (card != player->m_characters.front() && card->color != card_color_type::orange)
            return _("ERROR_TARGET_NOT_CUBE_SLOT");
    } else if (card->deck == card_deck_type::character) {
        return _("ERROR_TARGET_NOT_CARD");
    }

    if (bool(args.card_filter & target_card_filter::beer) && !card->has_tag(tag_type::beer))
        return _("ERROR_TARGET_NOT_BEER");

    if (bool(args.card_filter & target_card_filter::bang) && !is_bangcard(card))
        return _("ERROR_TARGET_NOT_BANG");
    
    if (bool(args.card_filter & target_card_filter::missed) && !card->has_tag(tag_type::missedcard))
        return _("ERROR_TARGET_NOT_MISSED");

    if (bool(args.card_filter & target_card_filter::bronco) && !card->has_tag(tag_type::bronco))
        return _("ERROR_TARGET_NOT_BRONCO");
    
    if (bool(args.card_filter & target_card_filter::blue) && card->color != card_color_type::blue)
        return _("ERROR_TARGET_NOT_BLUE_CARD");
    
    if (bool(args.card_filter & target_card_filter::clubs) && card->sign.suit != card_suit::clubs)
        return _("ERROR_TARGET_NOT_CLUBS");
    
    if (bool(args.card_filter & target_card_filter::black) != (card->color == card_color_type::black))
        return _("ERROR_TARGET_BLACK_CARD");
    
    if (bool(args.card_filter & target_card_filter::table) && card->pocket != &player->table)
        return _("ERROR_TARGET_NOT_TABLE_CARD");

    if (bool(args.card_filter & target_card_filter::hand) && card->pocket != &player->hand)
        return _("ERROR_TARGET_NOT_HAND_CARD");

    return std::nullopt;
}

std::vector<player_view *> target_finder::possible_player_targets(target_player_filter filter) {
    std::vector<player_view *> ret;
    for (auto &p : m_game->m_players) {
        if ((get_current_card()->has_tag(tag_type::can_repeat)
            || !ranges_contains(m_targets, target_variant(&p)))
            && !verify_player_target(filter, &p))
        {
            ret.push_back(&p);
        }
    }
    return ret;
}

void target_finder::add_card_target(player_view *player, card_view *card) {
    if (m_equipping) return;

    int index = get_target_index();
    auto cur_target = get_effect_holder(index);
    
    switch (cur_target.target) {
    case target_type::card:
        if (auto error = verify_card_target(cur_target, player, card)) {
            m_game->parent->add_chat_message(message_type::error, *error);
            os_api::play_bell();
        } else if (get_current_card()->has_tag(tag_type::can_repeat)
            || std::ranges::none_of(m_targets, [card](const target_variant &target) {
                return enums::visit(overloaded{
                    [](const auto &) { return false; },
                    [card](card_view *c) { return c == card; },
                    [card](nullable<card_view> c) { return c == nullable(card); }
                }, target);
            }))
        {
            if (player != m_game->m_player_self && card->pocket == &player->hand) {
                for (card_view *hand_card : player->hand) {
                    m_target_borders.add(hand_card->border_color, options.target_finder_target);
                }
            } else {
                m_target_borders.add(card->border_color, options.target_finder_target);
            }
            m_targets.emplace_back(card);
            handle_auto_targets();
        }
        break;
    case target_type::extra_card:
        if (player != m_game->m_player_self) {
            m_game->parent->add_chat_message(message_type::error, _("ERROR_TARGET_NOT_SELF"));
            os_api::play_bell();
        } else if (card->pocket != &player->hand) {
            m_game->parent->add_chat_message(message_type::error, _("ERROR_TARGET_NOT_HAND_CARD"));
            os_api::play_bell();
        } else if (card == m_playing_card) {
            m_game->parent->add_chat_message(message_type::error, _("ERROR_TARGET_PLAYING_CARD"));
            os_api::play_bell();
        } else {
            m_target_borders.add(card->border_color, options.target_finder_target);
            m_targets.emplace_back(nullable(card));
            handle_auto_targets();
        }
        break;
    case target_type::cards_other_players:
        if (index >= m_targets.size()) {
            m_targets.emplace_back(enums::enum_tag<target_type::cards_other_players>);
            m_targets.back().get<target_type::cards_other_players>().reserve(
                std::ranges::count_if(m_game->m_players, [&](const player_view &p) {
                    if (!p.alive() || &p == m_game->m_player_self) return false;
                    if (p.table.empty() && p.hand.empty()) return false;
                    return true;
                }));
        }
        if (auto &vec = m_targets.back().get<target_type::cards_other_players>();
            card->color != card_color_type::black && player != m_game->m_player_self
            && !ranges_contains(vec, player, &player_card_pair::first))
        {
            if (player != m_game->m_player_self && card->pocket == &player->hand) {
                for (card_view *hand_card : player->hand) {
                    m_target_borders.add(hand_card->border_color, options.target_finder_target);
                }
            } else {
                m_target_borders.add(card->border_color, options.target_finder_target);
            }
            vec.emplace_back(player, card);
            if (vec.size() == vec.capacity()) {
                handle_auto_targets();
            }
        }
        break;
    case target_type::cube:
        if (int ncubes = count_selected_cubes(card);
            ncubes < card->cubes.size() && player == m_game->m_player_self) {
            if (index >= m_targets.size()) {
                m_targets.emplace_back(enums::enum_tag<target_type::cube>);
                m_targets.back().get<target_type::cube>().reserve(std::max<int>(1, cur_target.effect_value));
            }
            auto &vec = m_targets.back().get<target_type::cube>();
            auto *cube = (card->cubes.rbegin() + ncubes)->get();
            m_target_borders.add(cube->border_color, options.target_finder_target);
            vec.emplace_back(card, cube);
            if (vec.size() == vec.capacity()) {
                handle_auto_targets();
            }
        }
        break;
    }
}

void target_finder::send_play_card() {
    play_card_args ret;
    ret.card_id = m_playing_card ? m_playing_card->id : 0;
    for (card_view *card : m_modifiers) {
        ret.modifier_ids.push_back(card->id);
    }

    for (const auto &target : m_targets) {
        ret.targets.push_back(enums::visit_indexed(overloaded{
            []<target_type E>(enums::enum_tag_t<E> tag) {
                return play_card_target_id(tag);
            },
            [](enums::enum_tag_t<target_type::player> tag, player_view *target) {
                return play_card_target_id(tag, target->id);
            },
            [](enums::enum_tag_t<target_type::conditional_player> tag, nullable<player_view> target) {
                return play_card_target_id(tag, target ? target->id : 0);
            },
            [](enums::enum_tag_t<target_type::card> tag, card_view *target) {
                return play_card_target_id(tag, target->id);
            },
            [](enums::enum_tag_t<target_type::extra_card> tag, nullable<card_view> target) {
                return play_card_target_id(tag, target ? target->id : 0);
            },
            [](enums::enum_tag_t<target_type::cards_other_players> tag, const std::vector<player_card_pair> &cs) {
                std::vector<int> ids;
                for (const auto &[player, card] : cs) {
                    ids.push_back(card->id);
                }
                return play_card_target_id(tag, std::move(ids));
            },
            [](enums::enum_tag_t<target_type::cube> tag, const std::vector<card_cube_pair> &cs) {
                std::vector<int> ids;
                for (const auto &[card, cube] : cs) {
                    ids.push_back(card->id);
                }
                return play_card_target_id(tag, std::move(ids));
            }
        }, target));
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

void target_finder::confirm_play() {
    m_game->m_ui.close_message_box();
    clear_targets();
    m_waiting_confirm = false;
}