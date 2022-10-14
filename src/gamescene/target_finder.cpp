#include "target_finder.h"

#include "game.h"
#include "../manager.h"
#include "../os_api.h"
#include "utils/utils.h"
#include "game/effect_list_zip.h"
#include "game/filters.h"

#include <cassert>
#include <numeric>

using namespace banggame;
using namespace enums::flag_operators;
using namespace sdl::point_math;

void target_finder::set_playing_card(card_view *card) {
    m_response = m_response && !bool(m_request_flags & effect_flags::force_play);
    m_playing_card = card;

    if (m_equipping || !get_current_card_effects().empty()
        && std::ranges::none_of(get_current_card_effects(), [&](const effect_holder &effect) {
            return effect.target == target_type::self_cubes && effect.target_value > static_cast<int>(card->cubes.size()) - count_selected_cubes(card);
        })
    ) {
        m_target_borders.add(card->border_color, colors.target_finder_current_card);
        handle_auto_targets();
    } else {
        m_response = false;
        m_playing_card = nullptr;
    }
}

template<game_action_type T>
void target_finder::add_action(auto && ... args) {
    m_game->parent->add_message<banggame::client_message_type::game_action>(json::serialize(banggame::game_action{enums::enum_tag<T>, FWD(args) ...}, *m_game));
}

void target_finder::set_picking_border(pocket_type pocket, player_view *player, card_view *card, sdl::color color) {
    switch (pocket) {
    case pocket_type::player_hand:
    case pocket_type::player_table:
    case pocket_type::player_character:
    case pocket_type::selection:
        m_response_borders.add(card->border_color, color);
        break;
    case pocket_type::main_deck:
        m_response_borders.add(m_game->m_main_deck.border_color, color);
        break;
    case pocket_type::discard_pile:
        m_response_borders.add(m_game->m_discard_pile.border_color, color);
        break;
    }
}

void target_finder::set_response_highlights(const request_status_args &args) {
    if (m_game->m_player_self && bool(args.flags & effect_flags::timer)) {
        add_action<game_action_type::request_confirm>();
    }
    clear_status();

    for (card_view *card : args.highlight_cards) {
        m_response_borders.add(card->border_color, colors.target_finder_highlight_card);
    }

    if (args.origin_card) {
        m_response_borders.add(args.origin_card->border_color, colors.target_finder_origin_card);
    }

    for (const auto &[pocket, player, card] : (m_picking_highlights = args.pick_cards)) {
        set_picking_border(pocket, player, card, colors.target_finder_can_pick);
    }

    for (card_view *card : (m_response_highlights = args.respond_cards)) {
        m_response_borders.add(card->border_color, colors.target_finder_can_respond);
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
            m_equipping = true;
            set_playing_card(card);
        } else if (card->modifier != card_modifier_type::none) {
            add_modifier(card);
        } else if (playable_with_modifiers(card)) {
            m_response = true;
            set_playing_card(card);
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

bool target_finder::is_card_clickable() const {
    return m_game->m_winner_role == player_role::unknown && m_game->m_pending_updates.empty() && m_game->m_animations.empty() && !waiting_confirm();
}

bool target_finder::can_respond_with(card_view *card) const {
    return ranges_contains(m_response_highlights, card);
}

bool target_finder::can_pick_card(pocket_type pocket, player_view *player, card_view *card) const {
    return ranges_contains(m_picking_highlights, picking_args{pocket, player, card});
}

bool target_finder::can_play_in_turn(pocket_type pocket, player_view *player, card_view *card) const {
    if (bool(m_request_flags & effect_flags::force_play)) {
        return can_respond_with(card) || card->pocket == &m_game->m_shop_choice;
    } else if (m_game->m_request_origin || m_game->m_request_target
        || m_game->m_playing != m_game->m_player_self
        || (player && player != m_game->m_player_self))
    {
        return false;
    }
    
    switch (pocket) {
    case pocket_type::player_hand:
    case pocket_type::player_character:
    case pocket_type::scenario_card:
    case pocket_type::button_row:
        return true;
    case pocket_type::player_table:
        return !card->inactive;
    case pocket_type::shop_selection:
        return m_game->m_player_self->gold >= card->buy_cost() - ranges_contains(m_modifiers, card_modifier_type::discount, &card_view::modifier);
    default:
        return false;
    }
}

void target_finder::on_click_card(pocket_type pocket, player_view *player, card_view *card) {
    if (card && card->has_tag(tag_type::confirm) && can_confirm()) {
        send_play_card();
    } else if (m_playing_card) {
        if (pocket == pocket_type::player_character) {
            add_card_target(player, player->m_characters.front());
        } else if (pocket == pocket_type::player_table || pocket == pocket_type::player_hand) {
            add_card_target(player, card);
        }
    } else if (can_respond_with(card)) {
        m_response = true;
        set_playing_card(card);
    } else if (can_pick_card(pocket, player, card)) {
        send_pick_card(pocket, player, card);
    } else if (can_play_in_turn(pocket, player, card)) {
        if ((pocket == pocket_type::player_hand || pocket == pocket_type::shop_selection) && card->color != card_color_type::brown) {
            if (playable_with_modifiers(card)) {
                m_equipping = true;
                set_playing_card(card);
            }
        } else if (card->modifier != card_modifier_type::none) {
            add_modifier(card);
        } else if (playable_with_modifiers(card)) {
            set_playing_card(card);
        }
    }
}

bool target_finder::on_click_player(player_view *player) {
    if (!m_game->m_player_self) {
        m_game->parent->add_message<client_message_type::lobby_rejoin>(player->id);
        return true;
    }

    auto verify_filter = [&](target_player_filter filter) {
        if (auto error = check_player_filter(filter, player))  {
            m_game->parent->add_chat_message(message_type::error, _(error));
            os_api::play_bell();
            return false;
        }
        return true;
    };

    if (!m_playing_card) {
        return false;
    } else if (m_equipping) {
        if (verify_filter(m_playing_card->equip_target)) {
            m_target_borders.add(player->border_color, colors.target_finder_target);
            m_targets.emplace_back(enums::enum_tag<target_type::player>, player);
            send_play_card();
            return true;
        }
    } else if (const auto &args = get_effect_holder(get_target_index()); !verify_filter(args.player_filter)) {
        return true;
    } else if (args.target == target_type::player || args.target == target_type::conditional_player) {
        if (ranges_contains(possible_player_targets(args.player_filter), player)) {
            m_target_borders.add(player->border_color, colors.target_finder_target);
            if (args.target == target_type::player) {
                m_targets.emplace_back(enums::enum_tag<target_type::player>, player);
            } else {
                m_targets.emplace_back(enums::enum_tag<target_type::conditional_player>, player);
            }
            handle_auto_targets();
        }
        return true;
    }
    return false;
}

void target_finder::add_modifier(card_view *card) {
    if (!ranges_contains(m_modifiers, card) && [&]{
        switch (card->modifier) {
        case card_modifier_type::bangmod:
        case card_modifier_type::bandolier:
            return std::ranges::all_of(m_modifiers, [](const card_view *c) {
                return c->modifier == card_modifier_type::bangmod
                    || c->modifier == card_modifier_type::bandolier
                    || c->modifier == card_modifier_type::belltower;
            });
        case card_modifier_type::leevankliff:
            return m_modifiers.empty() && m_last_played_card;
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
                return true;
            }
            break;
        case card_modifier_type::belltower:
            return m_modifiers.empty() || m_modifiers.front()->modifier != card_modifier_type::leevankliff;
        }
        return false;
    }()) {
        for (const auto &e : card->effects) {
            if (e.target == target_type::self_cubes) {
                add_selected_cube(card, e.target_value);
            }
        }

        m_modifiers.push_back(card);
        m_target_borders.add(card->border_color, colors.target_finder_current_card);
    }
}

bool target_finder::is_bangcard(card_view *card) const {
    return (m_game->m_player_self->has_player_flags(player_flags::treat_missed_as_bang)
            && card->has_tag(tag_type::missedcard))
        || card->has_tag(tag_type::bangcard);
}

bool target_finder::playable_with_modifiers(card_view *card) {
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

int target_finder::get_target_index() {
    if (m_targets.empty()) {
        return 0;
    }
    return static_cast<int>(m_targets.size()) - enums::visit(overloaded{
        [](const auto &) {
            return 0;
        },
        []<typename T>(const std::vector<T> &value) {
            return static_cast<int>(value.size() != value.capacity());
        }
    }, m_targets.back());
}

int target_finder::calc_distance(player_view *from, player_view *to) const {
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
    if (m_equipping) {
        if (m_playing_card->self_equippable()) {
            send_play_card();
        }
        return;
    }

    using namespace enums::flag_operators;

    auto *current_card = get_current_card();

    auto &effects = get_current_card_effects();

    auto &optionals = current_card->optionals;
    auto repeatable = current_card->get_tag_value(tag_type::repeatable);

    bool auto_confirmable = false;
    if (can_confirm()) {
        if (current_card->has_tag(tag_type::auto_confirm)) {
            auto_confirmable = std::ranges::any_of(optionals, [&](const effect_holder &holder) {
                return holder.target == target_type::player
                    && possible_player_targets(holder.player_filter).empty();
            });
        } else if (current_card->has_tag(tag_type::auto_confirm_red_ringo)) {
            auto_confirmable = current_card->cubes.size() <= 1
                || std::transform_reduce(m_game->m_playing->table.begin(), m_game->m_playing->table.end(), 0, std::plus(), [](const card_view *card) {
                    return 4 - static_cast<int>(card->cubes.size());
                }) <= 1;
        } else if (repeatable && *repeatable) {
            auto_confirmable = m_targets.size() - effects.size() == optionals.size() * *repeatable;
        }
    }
    
    if (auto_confirmable) {
        send_play_card();
        return;
    }

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
        switch (effect_it->target) {
        case target_type::none:
            m_targets.emplace_back(enums::enum_tag<target_type::none>);
            break;
        case target_type::conditional_player:
            if (possible_player_targets(effect_it->player_filter).empty()) {
                m_targets.emplace_back(enums::enum_tag<target_type::conditional_player>);
                break;
            } else {
                return;
            }
        case target_type::extra_card:
            if (current_card == m_last_played_card) {
                m_targets.emplace_back(enums::enum_tag<target_type::extra_card>);
                break;
            } else {
                return;
            }
        case target_type::other_players:
            m_targets.emplace_back(enums::enum_tag<target_type::other_players>);
            break;
        case target_type::all_players:
            m_targets.emplace_back(enums::enum_tag<target_type::all_players>);
            break;
        case target_type::self_cubes:
            add_selected_cube(m_playing_card, effect_it->target_value);
            m_targets.emplace_back(enums::enum_tag<target_type::self_cubes>);
            break;
        default:
            return;
        }
        ++effect_it;
    }
}

const char *target_finder::check_player_filter(target_player_filter filter, player_view *target_player) {
    if (ranges_contains(m_targets, play_card_target(enums::enum_tag<target_type::player>, target_player))) {
        return "ERROR_TARGET_NOT_UNIQUE";    
    } else {
        return banggame::check_player_filter(m_game->m_player_self, filter, target_player);
    }
}

const char *target_finder::check_card_filter(target_card_filter filter, card_view *card) {
    if (!bool(filter & target_card_filter::can_repeat)
        && std::ranges::any_of(m_targets, [card](const play_card_target &target) {
            return enums::visit(overloaded{
                [](const auto &) { return false; },
                [card](card_view *c) { return c == card; },
                [card](const std::vector<card_view *> &cs) { return ranges_contains(cs, card); },
            }, target);
        }))
    {
        return "ERROR_TARGET_NOT_UNIQUE";
    } else {
        return banggame::check_card_filter(m_playing_card, m_game->m_player_self, filter, card);
    }
}

std::vector<player_view *> target_finder::possible_player_targets(target_player_filter filter) {
    std::vector<player_view *> ret;
    for (auto &p : m_game->m_players) {
        if (!check_player_filter(filter, &p)) {
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
    case target_type::extra_card:
    case target_type::cards:
        if (auto error = check_card_filter(cur_target.card_filter, card)) {
            m_game->parent->add_chat_message(message_type::error, _(error));
            os_api::play_bell();
        } else {
            if (player != m_game->m_player_self && card->pocket == &player->hand) {
                for (card_view *hand_card : player->hand) {
                    m_target_borders.add(hand_card->border_color, colors.target_finder_target);
                }
            } else {
                m_target_borders.add(card->border_color, colors.target_finder_target);
            }
            if (cur_target.target == target_type::card) {
                m_targets.emplace_back(enums::enum_tag<target_type::card>, card);
                handle_auto_targets();
            } else if (cur_target.target == target_type::extra_card) {
                m_targets.emplace_back(enums::enum_tag<target_type::extra_card>, card);
                handle_auto_targets();
            } else {
                if (index >= m_targets.size()) {
                    m_targets.emplace_back(enums::enum_tag<target_type::cards>);
                    m_targets.back().get<target_type::cards>().reserve(std::max<int>(1, cur_target.target_value));
                }
                auto &vec = m_targets.back().get<target_type::cards>();
                vec.push_back(card);
                if (vec.size() == vec.capacity()) {
                    handle_auto_targets();
                }
            }
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
            && !ranges_contains(vec, player, [](card_view *card) { return card->pocket->owner; }))
        {
            if (player != m_game->m_player_self && card->pocket == &player->hand) {
                for (card_view *hand_card : player->hand) {
                    m_target_borders.add(hand_card->border_color, colors.target_finder_target);
                }
            } else {
                m_target_borders.add(card->border_color, colors.target_finder_target);
            }
            vec.emplace_back(card);
            if (vec.size() == vec.capacity()) {
                handle_auto_targets();
            }
        }
        break;
    case target_type::select_cubes:
        if (player == m_game->m_player_self) {
            if (add_selected_cube(card, 1)) {
                if (index >= m_targets.size()) {
                    m_targets.emplace_back(enums::enum_tag<target_type::select_cubes>);
                    m_targets.back().get<target_type::select_cubes>().reserve(std::max<int>(1, cur_target.target_value));
                }
                auto &vec = m_targets.back().get<target_type::select_cubes>();
                vec.emplace_back(card);
                if (vec.size() == vec.capacity()) {
                    handle_auto_targets();
                }
            }
        }
        break;
    }
}

int target_finder::count_selected_cubes(card_view *card) {
    int selected = 0;
    if (get_current_card()) {
        for (const auto &[target, effect] : zip_card_targets(m_targets, get_current_card_effects(), get_current_card()->optionals)) {
            if (const std::vector<card_view *> *val = target.get_if<target_type::select_cubes>()) {
                selected += static_cast<int>(std::ranges::count(*val, card));
            } else if (target.is(target_type::self_cubes)) {
                if (card == m_playing_card) {
                    selected += effect.target_value;
                }
            }
        }
    }
    for (card_view *mod_card : m_modifiers) {
        for (const auto &t : mod_card->effects) {
            if (t.target == target_type::self_cubes && card == mod_card) {
                selected += t.target_value;
            }
        }
    }
    return selected;
}

bool target_finder::add_selected_cube(card_view *card, int ncubes) {
    int selected = count_selected_cubes(card);

    if (ncubes > static_cast<int>(card->cubes.size()) - selected)
        return false;

    for (int i=0; i < ncubes; ++i) {
        cube_widget *cube = (card->cubes.rbegin() + selected + i)->get();
        m_target_borders.add(cube->border_color, colors.target_finder_target);
    }
    return true;
}

void target_finder::send_play_card() {
    if (m_response) {
        add_action<game_action_type::respond_card>(m_playing_card, m_modifiers, m_targets);
    } else {
        add_action<game_action_type::play_card>(m_playing_card, m_modifiers, m_targets);
    }

    m_waiting_confirm = true;
}

void target_finder::send_pick_card(pocket_type pocket, player_view *player, card_view *card) {
    set_picking_border(pocket, player, card, colors.target_finder_picked);
    add_action<game_action_type::pick_card>(pocket, player, card);
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