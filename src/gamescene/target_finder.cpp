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

    m_target_borders.emplace_back(card, game_style::current_card);
    set_request_borders();
    handle_auto_targets();
}

void target_finder::select_equip_card(card_view *card) {
    m_playing_card = card;
    m_mode = target_mode::equip;
    set_request_borders();

    m_target_borders.emplace_back(card, game_style::current_card);
    if (card->self_equippable()) {
        send_play_card();
    } else {
        for (player_view *p : m_game->m_alive_players) {
            if (is_valid_target(m_playing_card->equip_target, p)) {
                m_targetable_borders.emplace_back(p, game_style::targetable);
            }
        }
    }
}

template<game_action_type T>
void target_finder::add_action(auto && ... args) {
    m_game->parent->add_message<banggame::client_message_type::game_action>(json::serialize(banggame::game_action{enums::enum_tag<T>, FWD(args) ...}, m_game->context()));
}

void target_finder::set_request_borders() {
    m_request_borders.clear();

    if (!m_playing_card) {
        if (m_modifiers.empty()) {
            for (card_view *card : m_pick_cards) {
                switch (card->pocket->type) {
                case pocket_type::main_deck: card = m_game->m_main_deck.back(); break;
                case pocket_type::discard_pile: card = m_game->m_discard_pile.back(); break;
                }
                m_request_borders.emplace_back(card, game_style::pickable);
            }
        }
        for (const auto &[card, branches] : get_current_tree()) {
            m_request_borders.emplace_back(card, game_style::playable);
        }
    }
}

void target_finder::set_response_cards(const request_status_args &args) {
    for (card_view *card : args.highlight_cards) {
        m_highlights.emplace_back(card, game_style::highlight);
    }

    if (card_view *card = args.origin_card) {
        m_highlights.emplace_back(card, game_style::origin_card);
    }

    m_request_origin_card = args.origin_card;
    m_request_origin = args.origin;
    m_request_target = args.target;
    m_auto_select = args.auto_select;
    m_response = true;

    m_pick_cards = args.pick_cards;
    m_play_cards = args.respond_cards;
    m_distances = args.distances;

    set_request_borders();
    handle_auto_select();
}

void target_finder::set_play_cards(const status_ready_args &args) {
    m_play_cards = args.play_cards;
    m_distances = args.distances;
    set_request_borders();
}

void target_finder::clear_status() {
    clear_targets();
    static_cast<request_status &>(*this) = {};
}

void target_finder::clear_targets() {
    m_game->m_card_choice.clear();
    static_cast<target_status &>(*this) = {};
    set_request_borders();
}

void target_finder::handle_auto_select() {
    if (m_auto_select && m_mode == target_mode::start && m_play_cards.size() == 1 && m_pick_cards.empty()) {
        select_playing_card(m_play_cards.front().card);
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

bool target_finder::is_card_clickable() const {
    return m_game->m_player_self
        && !m_game->has_game_flags(game_flags::game_over)
        && m_game->m_pending_updates.empty()
        && m_game->m_animations.empty()
        && !m_game->m_ui.is_message_box_open()
        && !finished();
}

const card_modifier_tree &target_finder::get_current_tree() const {
    const card_modifier_tree *tree = &m_play_cards;
    for (const auto &[mod_card, targets] : m_modifiers) {
        if (auto it = std::ranges::find(*tree, mod_card, &card_modifier_node::card); it != tree->end()) {
            tree = &it->branches;
        } else {
            throw std::runtime_error("Cannot find modifier card");
        }
    }
    return *tree;
}

bool target_finder::can_play_card(card_view *target_card) const {
    return ranges::contains(get_current_tree(), target_card, &card_modifier_node::card);
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
            m_target_borders.emplace_back(card, game_style::current_card);
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
    if (m_mode == target_mode::equip) {
        if (is_valid_target(m_playing_card->equip_target, player)) {
            m_target_borders.emplace_back(player, game_style::selected_target);
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
            if (is_valid_target(cur_target.player_filter, player)) {
                if (cur_target.target == target_type::player) {
                    targets.emplace_back(enums::enum_tag<target_type::player>, player);
                    if (m_mode == target_mode::modifier && cur_target.type == effect_type::ctx_add) {
                        add_modifier_context(current_card, player, nullptr);
                    }
                } else {
                    targets.emplace_back(enums::enum_tag<target_type::conditional_player>, player);
                }
                m_target_borders.emplace_back(player, game_style::selected_target);
                handle_auto_targets();
            }
            return true;
        }
    }
    return false;
}

int target_finder::calc_distance(player_view *from, player_view *to) const {
    if (from != to) {
        auto it = std::ranges::find(m_distances.distances, to, &player_distance_item::player);
        if (it != m_distances.distances.end()) {
            return it->distance;
        }
    }
    return 0;
}

struct targetable_for_cards_other_player {
    player_view *origin;
    player_view *skipped_player;

    bool operator()(player_view *target) const {
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
                    && std::ranges::none_of(m_game->m_alive_players, make_target_check(holder.player_filter));
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

    m_targetable_borders.clear();

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
            if (std::ranges::none_of(m_game->m_alive_players, make_target_check(effect_it->player_filter))) {
                targets.emplace_back(enums::enum_tag<target_type::conditional_player>);
                break;
            }
            [[fallthrough]];
        case target_type::player:
            for (player_view *p : m_game->m_alive_players) {
                if (is_valid_target(effect_it->player_filter, p)) {
                    m_targetable_borders.emplace_back(p, game_style::targetable);
                }
            }
            return;
        case target_type::extra_card:
            if (m_context.repeat_card) {
                targets.emplace_back(enums::enum_tag<target_type::extra_card>);
                break;
            }
            [[fallthrough]];
        case target_type::card:
        case target_type::cards:
            for (card_view *c : ranges::views::concat(
                m_game->m_alive_players
                    | ranges::views::filter(make_target_check(effect_it->player_filter))
                    | ranges::views::for_each([&](player_view *p) {
                        return ranges::views::concat(p->hand, p->table)
                            | ranges::views::filter(make_target_check(effect_it->card_filter));
                    }),
                m_game->m_selection
            )) {
                m_targetable_borders.emplace_back(c, game_style::targetable);
            }
            return;
        case target_type::cards_other_players:
            if (auto targetable = m_game->m_alive_players
                | ranges::views::filter(targetable_for_cards_other_player{m_game->m_player_self, m_context.skipped_player}))
            {
                for (card_view *c : targetable | ranges::views::for_each([&](player_view *p) {
                    return ranges::views::concat(
                        p->table | ranges::views::remove_if(&card_view::is_black),
                        p->hand
                    );
                })) {
                    m_targetable_borders.emplace_back(c, game_style::targetable);
                }
                return;
            } else {
                targets.emplace_back(enums::enum_tag<target_type::cards_other_players>);
                break;
            }
        case target_type::players:
            targets.emplace_back(enums::enum_tag<target_type::players>);
            break;
        case target_type::select_cubes:
            for (card_view *c : ranges::views::concat(
                m_game->m_player_self->table,
                m_game->m_player_self->m_characters | ranges::views::take(1)
            )) {
                if (!c->cubes.empty()) {
                    for (auto &cube : c->cubes
                        | std::views::take(int(c->cubes.size()) - count_selected_cubes(c)))
                    {
                        m_targetable_borders.emplace_back(cube.get(), game_style::targetable);
                    }
                }
            }
            return;
        case target_type::self_cubes:
            add_selected_cube(current_card, effect_it->target_value);
            targets.emplace_back(enums::enum_tag<target_type::self_cubes>);
            break;
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
        ranges::views::for_each(value.m_modifiers, &modifier_pair::targets),
        ranges::views::transform(value.m_modifiers, [](const modifier_pair &pair) {
            return play_card_target{enums::enum_tag<target_type::card>, pair.card};
        })
    );
}

target_finder::player_target_check target_finder::make_target_check(target_player_filter filter) const {
    return player_target_check{*this, m_game->m_player_self, filter};
}

target_finder::card_target_check target_finder::make_target_check(target_card_filter filter) const {
    return card_target_check{*this, m_game->m_player_self, filter};
}

bool target_finder::player_target_check::operator ()(player_view *target_player) const {
    return !contains_element{target_player}(all_targets(status))
        && !filters::check_player_filter(origin, filter, target_player, status.m_context);
}

bool target_finder::card_target_check::operator ()(card_view *target_card) const {
    return ((bool(filter & target_card_filter::can_repeat) || !contains_element{target_card}(all_targets(status))))
        && !filters::check_card_filter(status.m_playing_card, origin, filter, target_card, status.m_context);
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
        if ((!player || is_valid_target(cur_target.player_filter, player)) && is_valid_target(cur_target.card_filter, card)) {
            if (player && player != m_game->m_player_self && card->pocket == &player->hand) {
                for (card_view *hand_card : player->hand) {
                    m_target_borders.emplace_back(hand_card, game_style::selected_target);
                }
            } else {
                m_target_borders.emplace_back(card, game_style::selected_target);
            }
            if (cur_target.target == target_type::card) {
                targets.emplace_back(enums::enum_tag<target_type::card>, card);
                if (m_mode == target_mode::modifier && cur_target.type == effect_type::ctx_add) {
                    add_modifier_context(current_card, nullptr, card);
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
            !card->is_black() && player != m_game->m_player_self && player != m_context.skipped_player
            && !ranges::contains(vec, player, [](card_view *card) { return card->pocket->owner; }))
        {
            if (player != m_game->m_player_self && card->pocket == &player->hand) {
                for (card_view *hand_card : player->hand) {
                    m_target_borders.emplace_back(hand_card, game_style::selected_target);
                }
            } else {
                m_target_borders.emplace_back(card, game_style::selected_target);
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
        m_target_borders.emplace_back(cube, game_style::selected_target);
    }
    return true;
}

void target_finder::add_modifier_context(card_view *mod_card, player_view *target_player, card_view *target_card) {
    switch (mod_card->modifier.type) {
    case modifier_type::belltower:
        m_context.ignore_distances = true;
        break;
    case modifier_type::card_choice:
        m_game->m_card_choice.set_anchor(mod_card, get_current_tree());
        break;
    case modifier_type::leevankliff:
    case modifier_type::moneybag:
        select_playing_card(m_context.repeat_card = get_current_tree().front().card);
        break;
    case modifier_type::traincost:
        if (mod_card->pocket->type == pocket_type::stations) {
            select_equip_card(get_current_tree().front().card);
        }
        break;
    case modifier_type::sgt_blaze:
        if (target_player) {
            m_context.skipped_player = target_player;
        }
        break;
    }
}

void target_finder::send_play_card() {
    if (m_mode == target_mode::modifier) {
        m_mode = target_mode::start;
        add_modifier_context(m_modifiers.back().card, nullptr, nullptr);
    } else {
        m_mode = target_mode::finish;
        add_action<game_action_type::play_card>(m_playing_card, m_modifiers, m_targets, m_game->manager()->get_config().bypass_prompt);
    }
}

void target_finder::send_pick_card(pocket_type pocket, player_view *player, card_view *card) {
    switch (pocket) {
    case pocket_type::main_deck: card = m_game->m_main_deck.back(); break;
    case pocket_type::discard_pile: card = m_game->m_discard_pile.back(); break;
    }
    if (card) {
        m_target_borders.emplace_back(card, game_style::picked);
        add_action<game_action_type::pick_card>(card, m_game->manager()->get_config().bypass_prompt);
        m_mode = target_mode::finish;
        m_picked_card = card;
    }
}

void target_finder::send_prompt_response(bool response) {
    if (response) {
        if (m_picked_card) {
            add_action<game_action_type::pick_card>(m_picked_card, true);
        } else {
            add_action<game_action_type::play_card>(m_playing_card, m_modifiers, m_targets, true);
        }
    } else {
        clear_targets();
        handle_auto_select();
    }
}