#include "target_finder.h"

#include "game.h"
#include "../manager.h"

#include "cards/effect_list_zip.h"
#include "cards/effect_enums.h"
#include "cards/game_enums.h"
#include "cards/filters.h"

#include "utils/utils.h"

#include <cassert>
#include <numeric>

using namespace banggame;
using namespace sdl::point_math;

static int get_target_index(const target_list &targets) {
    if (targets.empty()) {
        return 0;
    }
    return int(targets.size()) - enums::visit(overloaded{
        [](const auto &) {
            return 0;
        },
        []<typename T>(const std::vector<T> &value) {
            return int(value.size() != value.capacity());
        }
    }, targets.back());
}

static const effect_holder &get_effect_holder(const effect_list &effects, const effect_list &optionals, int index) {
    if (index < effects.size()) {
        return effects[index];
    }

    return optionals[(index - effects.size()) % optionals.size()];
}

card_view *target_status::get_current_card() const {
    switch (m_mode) {
    case target_mode::target: return m_playing_card;
    case target_mode::modifier: return m_modifiers.back().card;
    default: throw std::runtime_error("Invalid target mode");
    }
}

target_list &target_status::get_current_target_list() {
    switch (m_mode) {
    case target_mode::target: return m_targets;
    case target_mode::modifier: return m_modifiers.back().targets;
    default: throw std::runtime_error("Invalid target mode");
    }
}

const target_list &target_status::get_current_target_list() const {
    switch (m_mode) {
    case target_mode::target: return m_targets;
    case target_mode::modifier: return m_modifiers.back().targets;
    default: throw std::runtime_error("Invalid target mode");
    }
}

void target_finder::select_playing_card(card_view *card) {
    if (card->is_modifier()) {
        m_modifiers.emplace_back(card);
        m_mode = target_mode::modifier;
    } else {
        m_playing_card = card;
        m_mode = target_mode::target;
    }

    m_target_borders.add(card->border_color, colors.target_finder_current_card);
    handle_auto_targets();
}

void target_finder::select_equip_card(card_view *card) {
    m_playing_card = card;
    m_mode = target_mode::equip;

    m_target_borders.add(card->border_color, colors.target_finder_current_card);
    if (card->self_equippable()) {
        send_play_card();
    }
}

template<game_action_type T>
void target_finder::add_action(auto && ... args) {
    m_game->parent->add_message<banggame::client_message_type::game_action>(json::serialize(banggame::game_action{enums::enum_tag<T>, FWD(args) ...}, m_game->context()));
}

void target_finder::add_pick_border(card_view *card, sdl::color color) {
    switch (card->pocket->type) {
    case pocket_type::main_deck:
        m_request_borders.add(m_game->m_main_deck.border_color, color);
        break;
    case pocket_type::discard_pile:
        m_request_borders.add(m_game->m_discard_pile.border_color, color);
        break;
    default:
        m_request_borders.add(card->border_color, color);
    }
}

void target_finder::set_response_cards(const request_status_args &args) {
    for (card_view *card : args.highlight_cards) {
        m_request_borders.add(card->border_color, colors.target_finder_highlight_card);
    }

    if (card_view *c = args.origin_card) {
        m_request_borders.add(c->border_color, colors.target_finder_origin_card);
    }

    for (card_view *card : (m_pick_cards = args.pick_cards)) {
        add_pick_border(card, colors.target_finder_can_pick);
    }

    for (card_view *card : (m_play_cards = args.respond_cards)) {
        m_request_borders.add(card->border_color, colors.target_finder_can_respond);
    }

    m_request_flags = args.flags;
    m_response = true;
    handle_auto_respond();
}

void target_finder::set_play_cards(const status_ready_args &args) {
    for (card_view *card : (m_play_cards = args.play_cards)) {
        m_request_borders.add(card->border_color, colors.target_finder_can_respond);
    }

    m_last_played_card = args.last_played_card;
}

void target_finder::clear_status() {
    clear_targets();
    static_cast<request_status &>(*this) = {};
}

void target_finder::clear_targets() {
    m_game->m_card_choice.clear();
    static_cast<target_status &>(*this) = {};
}

void target_finder::handle_auto_respond() {
    if (m_mode == target_mode::start && bool(m_request_flags & effect_flags::auto_respond) && m_play_cards.size() == 1 && m_pick_cards.empty()) {
        select_playing_card(m_play_cards.front());
    }
}

bool target_finder::can_confirm() const {
    if (m_mode == target_mode::target || m_mode == target_mode::modifier) {
        card_view *current_card = get_current_card();
        const size_t neffects = current_card->get_effect_list(m_response).size();
        const size_t noptionals = current_card->optionals.size();
        const auto &targets = get_current_target_list();
        return noptionals != 0
            && targets.size() >= neffects
            && ((targets.size() - neffects) % noptionals == 0);
    }
    return false;
}

bool target_finder::is_card_selected() const {
    return m_playing_card || !m_modifiers.empty();
}

bool target_finder::is_card_clickable() const {
    return !m_game->has_game_flags(game_flags::game_over)
        && m_game->m_pending_updates.empty()
        && m_game->m_animations.empty()
        && !m_game->m_ui.is_message_box_open()
        && !finished();
}

bool target_finder::can_play_card(card_view *target_card) const {
    if (m_modifiers.empty()) {
        return ranges::contains(m_play_cards, target_card);
    } else {
        return filters::get_card_cost(target_card, m_response, m_context) <= m_game->m_playing->gold
            && std::ranges::all_of(m_modifiers, [&](card_view *mod_card) {
                return card_playable_with_modifier(mod_card, target_card);
            }, &modifier_pair::card);
    }
}

bool target_finder::can_pick_card(pocket_type pocket, player_view *player, card_view *card) const {
    switch (pocket) {
    case pocket_type::main_deck:
    case pocket_type::discard_pile:
        return ranges::contains(m_pick_cards, pocket, [](card_view *c) { return c->pocket->type; });
    default:
        return ranges::contains(m_pick_cards, card);
    }
}

void target_finder::on_click_card(pocket_type pocket, player_view *player, card_view *card) {
    if (card && card->has_tag(tag_type::confirm) && can_confirm()) {
        send_play_card();
    } else if (m_mode == target_mode::target || m_mode == target_mode::modifier) {
        switch (pocket) {
        case pocket_type::player_character:
            add_card_target(player, player->m_characters.front());
            break;
        case pocket_type::player_table:
        case pocket_type::player_hand:
        case pocket_type::selection:
            add_card_target(player, card);
            break;
        }
    } else if (m_response) {
        bool can_respond = can_play_card(card);
        bool can_pick = can_pick_card(pocket, player, card);
        if (can_respond && can_pick) {
            m_target_borders.add(card->border_color, colors.target_finder_current_card);
            m_game->m_ui.show_message_box(_("PROMPT_PLAY_OR_PICK"), {
                {_("BUTTON_PLAY"), [=, this]{ select_playing_card(card); }},
                {_("BUTTON_PICK"), [=, this]{ send_pick_card(pocket, player, card); }},
                {_("BUTTON_UNDO"), [this]{ clear_targets(); }}
            });
        } else if (can_respond) {
            select_playing_card(card);
        } else if (can_pick) {
            send_pick_card(pocket, player, card);
        }
    } else if (m_game->m_playing == m_game->m_player_self && (!player || player == m_game->m_player_self) && can_play_card(card)) {
        if (filters::is_equip_card(card)) {
            select_equip_card(card);
        } else {
            select_playing_card(card);
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
            m_game->play_sound("invalid");
            return false;
        }
        return true;
    };

    if (m_mode == target_mode::equip) {
        if (verify_filter(m_playing_card->equip_target)) {
            m_target_borders.add(player->border_color, colors.target_finder_target);
            m_targets.emplace_back(enums::enum_tag<target_type::player>, player);
            send_play_card();
        }
        return true;
    } else if (m_mode == target_mode::target || m_mode == target_mode::modifier) {
        card_view *current_card = get_current_card();
        auto &targets = get_current_target_list();
        int index = get_target_index(targets);
        auto cur_target = get_effect_holder(current_card->get_effect_list(m_response), current_card->optionals, index);

        if (cur_target.target == target_type::player || cur_target.target == target_type::conditional_player) {
            if (verify_filter(cur_target.player_filter)) {
                if (cur_target.target == target_type::player) {
                    targets.emplace_back(enums::enum_tag<target_type::player>, player);
                    if (m_mode == target_mode::modifier && cur_target.type == effect_type::ctx_add) {
                        add_modifier_context(current_card, player);
                    }
                } else {
                    targets.emplace_back(enums::enum_tag<target_type::conditional_player>, player);
                }
                m_target_borders.add(player->border_color, colors.target_finder_target);
                handle_auto_targets();
            }
            return true;
        } else if (!verify_filter(cur_target.player_filter)) {
            return true;
        }
    }
    return false;
}

int target_finder::calc_distance(player_view *from, player_view *to) const {
    if (from == to) return 0;

    if (m_context.ignore_distances) {
        return 1;
    }

    if (m_game->has_game_flags(game_flags::disable_player_distances)) {
        return 1 + to->m_distance_mod;
    }

    struct player_view_iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = int;
        using value_type = player_view *;
        using pointer = value_type *;
        using reference = value_type &;

        std::vector<banggame::player_view *> *list;
        std::vector<banggame::player_view *>::iterator m_it;

        player_view_iterator() = default;
        
        player_view_iterator(game_scene *game, player_view *p)
            : list(&game->m_alive_players)
            , m_it(std::ranges::find(*list, p))
        {
            assert(m_it != list->end());
        }
        
        player_view_iterator &operator ++() {
            do {
                ++m_it;
                if (m_it == list->end()) m_it = list->begin();
            } while (!(*m_it)->alive());
            return *this;
        }

        player_view_iterator operator ++(int) { auto copy = *this; ++*this; return copy; }

        player_view_iterator &operator --() {
            do {
                if (m_it == list->begin()) m_it = list->end();
                --m_it;
            } while (!(*m_it)->alive());
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

struct targetable_for_cards_other_player {
    player_view *origin;
    player_view *skipped_player;

    bool operator()(player_view *target) {
        return target != origin && target != skipped_player && target->alive()
            && (!target->hand.empty() || std::ranges::any_of(target->table, std::not_fn(&card_view::is_black)));
    }
};

void target_finder::handle_auto_targets() {
    auto *current_card = get_current_card();
    auto &effects = current_card->get_effect_list(m_response);
    auto &targets = get_current_target_list();

    auto &optionals = current_card->optionals;
    auto repeatable = current_card->get_tag_value(tag_type::repeatable);

    bool auto_confirmable = false;
    if (can_confirm()) {
        if (current_card->has_tag(tag_type::auto_confirm)) {
            auto_confirmable = std::ranges::any_of(optionals, [&](const effect_holder &holder) {
                return holder.target == target_type::player
                    && std::ranges::none_of(m_game->m_alive_players, [&](player_view *p) {
                        return !check_player_filter(holder.player_filter, p);
                    });
            });
        } else if (current_card->has_tag(tag_type::auto_confirm_red_ringo)) {
            auto_confirmable = current_card->cubes.size() <= 1
                || ranges::accumulate(m_game->m_player_self->table
                    | ranges::views::filter(&card_view::is_orange)
                    | ranges::views::transform([](const card_view *card) {
                        return 4 - int(card->cubes.size());
                    }), 0) <= 1;
        } else if (repeatable && *repeatable) {
            auto_confirmable = targets.size() - effects.size() == optionals.size() * *repeatable;
        }
    }
    
    if (auto_confirmable) {
        send_play_card();
        return;
    }

    auto effect_it = effects.data();
    auto target_end = effects.data() + effects.size();
    if (targets.size() >= effects.size() && !optionals.empty()) {
        size_t diff = targets.size() - effects.size();
        effect_it = optionals.data() + (repeatable ? diff % optionals.size() : diff);
        target_end = optionals.data() + optionals.size();
    } else {
        effect_it += targets.size();
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
            targets.emplace_back(enums::enum_tag<target_type::none>);
            break;
        case target_type::conditional_player:
            if (std::ranges::none_of(m_game->m_alive_players, [&](player_view *p) {
                return !check_player_filter(effect_it->player_filter, p);
            })) {
                targets.emplace_back(enums::enum_tag<target_type::conditional_player>);
                break;
            } else {
                return;
            }
        case target_type::extra_card:
            if (m_context.repeat_card) {
                targets.emplace_back(enums::enum_tag<target_type::extra_card>);
                break;
            } else {
                return;
            }
        case target_type::cards_other_players:
            if (std::ranges::none_of(m_game->m_alive_players,
                targetable_for_cards_other_player{m_game->m_player_self, m_context.skipped_player}))
            {
                targets.emplace_back(enums::enum_tag<target_type::cards_other_players>);
                break;
            } else {
                return;
            }
        case target_type::players:
            targets.emplace_back(enums::enum_tag<target_type::players>);
            break;
        case target_type::self_cubes:
            add_selected_cube(current_card, effect_it->target_value);
            targets.emplace_back(enums::enum_tag<target_type::self_cubes>);
            break;
        default:
            return;
        }
        ++effect_it;
    }
}

template<typename T> struct contains_element {
    const T &value;

    template<typename U>
    bool operator()(const U &other) const {
        return false;
    }
    
    template<std::equality_comparable_with<T> U>
    bool operator()(const U &other) const {
        return value == other;
    }

    template<std::ranges::range R> requires std::invocable<contains_element<T>, std::ranges::range_value_t<R>>
    bool operator()(R &&range) const {
        return std::ranges::any_of(std::forward<R>(range), *this);
    }

    template<enums::is_enum_variant U>
    bool operator()(const U &variant) const {
        return enums::visit(*this, variant);
    }
};

template<typename T> contains_element(const T &) -> contains_element<T>;

inline auto all_targets(const target_status &value) {
    return ranges::views::concat(
        ranges::views::all(value.m_targets),
        ranges::views::for_each(value.m_modifiers, &modifier_pair::targets)
    );
}

const char *target_finder::check_player_filter(target_player_filter filter, player_view *target_player) {
    if (contains_element{target_player}(all_targets(*this))) {
        return "ERROR_TARGET_NOT_UNIQUE";
    } else {
        return filters::check_player_filter(m_game->m_player_self, filter, target_player);
    }
}

const char *target_finder::check_card_filter(target_card_filter filter, card_view *card) {
    if (!bool(filter & target_card_filter::can_repeat) && contains_element{card}(all_targets(*this))) {
        return "ERROR_TARGET_NOT_UNIQUE";
    } else {
        return filters::check_card_filter(m_playing_card, m_game->m_player_self, filter, card);
    }
}

void target_finder::add_card_target(player_view *player, card_view *card) {
    if (m_mode == target_mode::equip) return;

    card_view *current_card = get_current_card();
    auto &targets = get_current_target_list();
    int index = get_target_index(targets);
    auto cur_target = get_effect_holder(current_card->get_effect_list(m_response), current_card->optionals, index);
    
    switch (cur_target.target) {
    case target_type::card:
    case target_type::extra_card:
    case target_type::cards:
        if (auto error = check_card_filter(cur_target.card_filter, card)) {
            m_game->parent->add_chat_message(message_type::error, _(error));
            m_game->play_sound("invalid");
        } else {
            if (player != m_game->m_player_self && card->pocket == &player->hand) {
                for (card_view *hand_card : player->hand) {
                    m_target_borders.add(hand_card->border_color, colors.target_finder_target);
                }
            } else {
                m_target_borders.add(card->border_color, colors.target_finder_target);
            }
            if (cur_target.target == target_type::card) {
                targets.emplace_back(enums::enum_tag<target_type::card>, card);
                if (m_mode == target_mode::modifier && cur_target.type == effect_type::ctx_add) {
                    add_modifier_context(current_card, card);
                }
                handle_auto_targets();
            } else if (cur_target.target == target_type::extra_card) {
                targets.emplace_back(enums::enum_tag<target_type::extra_card>, card);
                handle_auto_targets();
            } else {
                if (index >= targets.size()) {
                    targets.emplace_back(enums::enum_tag<target_type::cards>);
                    targets.back().get<target_type::cards>().reserve(std::max<int>(1, cur_target.target_value));
                }
                auto &vec = targets.back().get<target_type::cards>();
                vec.push_back(card);
                if (vec.size() == vec.capacity()) {
                    handle_auto_targets();
                }
            }
        }
        break;
    case target_type::cards_other_players:
        if (index >= targets.size()) {
            targets.emplace_back(enums::enum_tag<target_type::cards_other_players>);
            targets.back().get<target_type::cards_other_players>().reserve(
                std::ranges::count_if(m_game->m_alive_players,
                targetable_for_cards_other_player{m_game->m_player_self, m_context.skipped_player}));
        }
        if (auto &vec = targets.back().get<target_type::cards_other_players>();
            !card->is_black() && player != m_game->m_player_self
            && !ranges::contains(vec, player, [](card_view *card) { return card->pocket->owner; }))
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
                if (index >= targets.size()) {
                    targets.emplace_back(enums::enum_tag<target_type::select_cubes>);
                    targets.back().get<target_type::select_cubes>().reserve(std::max<int>(1, cur_target.target_value));
                }
                auto &vec = targets.back().get<target_type::select_cubes>();
                vec.emplace_back(card);
                if (vec.size() == vec.capacity()) {
                    handle_auto_targets();
                }
            }
        }
        break;
    }
}

int target_finder::count_selected_cubes(card_view *target_card) {
    int selected = 0;
    auto do_count = [&](card_view *card, const target_list &targets) {
        for (const auto &[target, effect] : zip_card_targets(targets, card, m_response)) {
            if (const std::vector<card_view *> *val = target.get_if<target_type::select_cubes>()) {
                selected += int(std::ranges::count(*val, target_card));
            } else if (target.is(target_type::self_cubes)) {
                if (card == target_card) {
                    selected += effect.target_value;
                }
            }
        }
    };
    if (m_playing_card) {
        do_count(m_playing_card, m_targets);
    }
    for (const auto &[card, targets] : m_modifiers) {
        do_count(card, targets);
    }
    return selected;
}

bool target_finder::add_selected_cube(card_view *card, int ncubes) {
    int selected = count_selected_cubes(card);

    if (ncubes > int(card->cubes.size()) - selected)
        return false;

    for (int i=0; i < ncubes; ++i) {
        cube_widget *cube = (card->cubes.rbegin() + selected + i)->get();
        m_target_borders.add(cube->border_color, colors.target_finder_target);
    }
    return true;
}

void target_finder::send_play_card() {
    if (m_mode == target_mode::modifier) {
        m_mode = target_mode::start;
        add_modifier_context(m_modifiers.back().card);
    } else {
        m_mode = target_mode::finish;
        add_action<game_action_type::play_card>(m_playing_card, m_modifiers, m_targets, m_response);
    }
}

void target_finder::send_pick_card(pocket_type pocket, player_view *player, card_view *card) {
    switch (pocket) {
    case pocket_type::main_deck:
        card = m_game->m_main_deck.back(); break;
    case pocket_type::discard_pile:
        card = m_game->m_discard_pile.back(); break;
    }
    if (card) {
        add_pick_border(card, colors.target_finder_picked);
        add_action<game_action_type::pick_card>(card);
        m_mode = target_mode::finish;
    }
}

void target_finder::send_prompt_response(bool response) {
    add_action<game_action_type::prompt_respond>(response);
    if (!response) {
        clear_targets();
        handle_auto_respond();
    }
}