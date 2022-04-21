#include "player.h"

#include "game.h"

#include "holders.h"
#include "play_verify.h"
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

        m_game->move_card(target, pocket_type::player_table, this, show_card_flags::shown);
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

    std::vector<card *>::iterator player::move_card_to(card *target_card, pocket_type pocket, player *target, show_card_flags flags) {
        if (target_card->owner == this) {
            if (target_card->pocket == pocket_type::player_table) {
                if (target_card->inactive) {
                    target_card->inactive = false;
                    m_game->add_public_update<game_update_type::tap_card>(target_card->id, false);
                }
                drop_all_cubes(target_card);
                auto it = m_game->move_card(target_card, pocket, target, flags);
                m_game->call_event<event_type::post_discard_card>(this, target_card);
                unequip_if_enabled(target_card);
                return it;
            } else if (target_card->pocket == pocket_type::player_hand) {
                return m_game->move_card(target_card, pocket, target, flags);
            }
        }
        throw std::runtime_error("Invalid card move");
    }

    void player::discard_card(card *target) {
        move_card_to(target, target->color == card_color_type::black
            ? pocket_type::shop_discard
            : pocket_type::discard_pile);
    }

    void player::steal_card(card *target) {
        target->owner->move_card_to(target, pocket_type::player_hand, this);
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

    void player::queue_request_add_cube(card *origin_card, int ncubes) {
        int nslots = 4 - m_characters.front()->cubes.size();
        int ncards = nslots > 0;
        for (card *c : m_table) {
            if (c->color == card_color_type::orange) {
                ncards += c->cubes.size() < 4;
                nslots += 4 - c->cubes.size();
            }
        }
        if (nslots <= ncubes || ncards <= 1) {
            auto do_add_cubes = [&](card *c) {
                int cubes_to_add = std::min<int>(ncubes, 4 - c->cubes.size());
                ncubes -= cubes_to_add;
                add_cubes(c, cubes_to_add);
            };
            do_add_cubes(m_characters.front());
            for (card *c : m_table) {
                if (c->color == card_color_type::orange) {
                    do_add_cubes(c);
                }
            }
        } else {
            m_game->queue_request<request_add_cube>(origin_card, this, ncubes);
        }
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

    void player::pay_cubes(card *origin, int ncubes) {
        move_cubes(origin, nullptr, ncubes);
    }

    void player::move_cubes(card *origin, card *target, int ncubes) {
        for(;ncubes!=0 && !origin->cubes.empty(); --ncubes) {
            int cube = origin->cubes.back();
            origin->cubes.pop_back();
            
            if (target && target->cubes.size() < 4) {
                target->cubes.push_back(cube);
                m_game->add_public_update<game_update_type::move_cube>(cube, target->id);
            } else {
                m_game->m_cubes.push_back(cube);
                m_game->add_public_update<game_update_type::move_cube>(cube, 0);
            }
        }
        if (origin->sign && origin->cubes.empty()) {
            m_game->move_card(origin, pocket_type::discard_pile);
            m_game->call_event<event_type::post_discard_orange_card>(this, origin);
            unequip_if_enabled(origin);
        }
    }

    void player::drop_all_cubes(card *target) {
        for (int id : target->cubes) {
            m_game->m_cubes.push_back(id);
            m_game->add_public_update<game_update_type::move_cube>(id, 0);
        }
        target->cubes.clear();
    }

    void player::add_to_hand(card *target) {
        m_game->move_card(target, pocket_type::player_hand, this);
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

    bool player::is_bangcard(card *card_ptr) {
        return (check_player_flags(player_flags::treat_missed_as_bang)
                && card_ptr->responses.last_is(effect_type::missedcard))
            || card_ptr->effects.last_is(effect_type::bangcard);
    };

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

    void player::play_card_action(card *card_ptr) {
        switch (card_ptr->pocket) {
        case pocket_type::player_hand:
            m_game->move_card(card_ptr, pocket_type::discard_pile);
            m_game->call_event<event_type::on_play_hand_card>(this, card_ptr);
            break;
        case pocket_type::player_table:
            if (card_ptr->color == card_color_type::green) {
                m_game->move_card(card_ptr, pocket_type::discard_pile);
            }
            break;
        case pocket_type::shop_selection:
            if (card_ptr->color == card_color_type::brown) {
                m_game->move_card(card_ptr, pocket_type::shop_discard);
            }
            break;
        default:
            break;
        }
    }

    void player::log_played_card(card *card_ptr, bool is_response) {
        m_game->send_card_update(card_ptr);
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
                card_ptr->responses.first_is(effect_type::drawing)
                    ? "LOG_DRAWN_WITH_CHARACTER"
                    : "LOG_RESPONDED_WITH_CHARACTER"
                : "LOG_PLAYED_CHARACTER", card_ptr, this);
            break;
        case pocket_type::shop_selection:
            m_game->add_log("LOG_BOUGHT_CARD", card_ptr, this);
            break;
        }
    }

    static std::vector<card *> find_cards(game *game, const std::vector<int> &args) {
        std::vector<card *> ret;
        for (int id : args) {
            ret.push_back(game->find_card(id));
        }
        return ret;
    }

    static target_list parse_target_id_vector(game *game, const std::vector<play_card_target_ids> &args) {
        target_list ret;
        for (const auto &t : args) {
            ret.push_back(enums::visit_indexed<play_card_target>(util::overloaded{
                [](enums::enum_tag_t<play_card_target_type::none>) {
                    return target_none_t{};
                },
                [](enums::enum_tag_t<play_card_target_type::other_players>) {
                    return target_other_players_t{};
                },
                [game](enums::enum_tag_t<play_card_target_type::player>, int player_id) {
                    return target_player_t{game->find_player(player_id)};
                },
                [game](enums::enum_tag_t<play_card_target_type::card>, int card_id) {
                    return target_card_t{game->find_card(card_id)};
                },
                [game](enums::enum_tag_t<play_card_target_type::cards_other_players>, const std::vector<int> &args) {
                    return target_cards_other_players_t{find_cards(game, args)};
                }
            }, t));
        }
        return ret;
    }

    void player::play_card(const play_card_args &args) {
        [[maybe_unused]] confirmer _confirm{this};
        m_prompt.reset();

        if (!m_game->m_requests.empty() || m_game->m_playing != this) {
            throw game_error("ERROR_INVALID_ACTION");
        }

        if (auto error = play_card_verify{
            this,
            m_game->find_card(args.card_id),
            false,
            parse_target_id_vector(m_game, args.targets),
            find_cards(m_game, args.modifier_ids)
        }.verify_and_play()) {
            throw std::move(*error);
        }
    }
    
    void player::respond_card(const play_card_args &args) {
        [[maybe_unused]] confirmer _confirm{this};
        m_prompt.reset();
        
        play_card_verify verifier{
            this,
            m_game->find_card(args.card_id),
            true,
            parse_target_id_vector(m_game, args.targets),
            find_cards(m_game, args.modifier_ids)
        };

        if (!can_respond_with(verifier.card_ptr)
            || (verifier.card_ptr->pocket == pocket_type::player_hand
            && verifier.card_ptr->color != card_color_type::brown)
            || (!verifier.modifiers.empty())
        ) {
            throw game_error("ERROR_INVALID_ACTION");
        }
        
        if (auto error = verifier.verify_card_targets()) {
            throw std::move(*error);
        }
        prompt_then(verifier.check_prompt(), [=, this]{
            verifier.do_play_card();
            set_last_played_card(nullptr);
        });
    }

    void player::prompt_then(opt_fmt_str &&message, std::function<void()> &&fun) {
        if (message) {
            m_game->add_private_update<game_update_type::game_prompt>(this, std::move(*message));
            m_prompt = std::move(fun);
        } else {
            m_game->add_private_update<game_update_type::confirm_play>(this);
            std::invoke(fun);
        }
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
            m_game->move_card(m_hand.front(), pocket_type::discard_pile);
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