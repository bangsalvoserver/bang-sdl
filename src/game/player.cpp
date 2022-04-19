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
        if (target_card->owner != this) throw game_error("ERROR_PLAYER_DOES_NOT_OWN_CARD");
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
        } else {
            throw game_error("ERROR_CARD_NOT_FOUND");
        }
    }

    void player::discard_card(card *target) {
        move_card_to(target, target->color == card_color_type::black
            ? pocket_type::shop_discard
            : pocket_type::discard_pile);
    }

    void player::steal_card(player *target, card *target_card) {
        target->move_card_to(target_card, pocket_type::player_hand, this);
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

    static void check_orange_card_empty(player *owner, card *target) {
        if (target->cubes.empty() && target->pocket != pocket_type::player_character && target->pocket != pocket_type::player_backup) {
            owner->m_game->move_card(target, pocket_type::discard_pile);
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

    static std::vector<card *> find_cards(game *game, const std::vector<int> &args) {
        std::vector<card *> ret;
        for (int id : args) {
            ret.push_back(game->find_card(id));
        }
        return ret;
    }

    static std::vector<play_card_target> parse_target_id_vector(game *game, const std::vector<play_card_target_ids> &args) {
        std::vector<play_card_target> ret;
        for (const auto &t : args) {
            ret.push_back(enums::visit_indexed<play_card_target>(util::overloaded{
                [](enums::enum_tag_t<play_card_target_type::none>) {
                    return target_none{};
                },
                [](enums::enum_tag_t<play_card_target_type::other_players>) {
                    return target_other_players{};
                },
                [game](enums::enum_tag_t<play_card_target_type::player>, int player_id) {
                    return target_player{game->find_player(player_id)};
                },
                [game](enums::enum_tag_t<play_card_target_type::card>, int card_id) {
                    card *c = game->find_card(card_id);
                    return target_card{c->owner, c};
                },
                [game](enums::enum_tag_t<play_card_target_type::cards_other_players>, const std::vector<int> &args) {
                    return target_cards_other_players{find_cards(game, args)};
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

        modifier_play_card_verify verifier{
            this,
            m_game->find_card(args.card_id),
            false,
            parse_target_id_vector(m_game, args.targets),
            find_cards(m_game, args.modifier_ids)
        };

        if (m_mandatory_card == verifier.card_ptr) {
            m_mandatory_card = nullptr;
        }
        
        if (m_forced_card) {
            if (verifier.card_ptr != m_forced_card && std::ranges::find(verifier.modifiers, m_forced_card) == verifier.modifiers.end()) {
                throw game_error("ERROR_INVALID_ACTION");
            } else {
                m_forced_card = nullptr;
            }
        }

        switch(verifier.card_ptr->pocket) {
        case pocket_type::player_hand:
            if (!verifier.modifiers.empty() && verifier.modifiers.front()->modifier == card_modifier_type::leevankliff) {
                // Uso il raii eliminare il limite di bang
                // quando lee van kliff gioca l'effetto del personaggio su una carta bang.
                // Se le funzioni di verifica throwano viene chiamato il distruttore
                struct banglimit_remover {
                    int8_t &num;
                    banglimit_remover(int8_t &num) : num(num) { ++num; }
                    ~banglimit_remover() { --num; }
                } _banglimit_remover{m_bangs_per_turn};
                if (m_game->is_disabled(verifier.modifiers.front())) {
                    throw game_error("ERROR_CARD_IS_DISABLED", verifier.modifiers.front());
                }
                play_card_verify leevankliff_verifier{this, m_last_played_card, false, verifier.targets};
                leevankliff_verifier.verify_card_targets();
                prompt_then(leevankliff_verifier.check_prompt(),
                    [this, card_ptr = verifier.card_ptr, leevankliff_verifier]{
                        m_game->move_card(card_ptr, pocket_type::discard_pile);
                        m_game->call_event<event_type::on_play_hand_card>(this, card_ptr);
                        leevankliff_verifier.do_play_card();
                        set_last_played_card(nullptr);
                    });
            } else if (verifier.card_ptr->color == card_color_type::brown) {
                if (m_game->is_disabled(verifier.card_ptr)) {
                    throw game_error("ERROR_CARD_IS_DISABLED", verifier.card_ptr);
                }
                verifier.verify_modifiers();
                verifier.verify_card_targets();
                prompt_then(verifier.check_prompt(), [this, verifier]{
                    verifier.play_modifiers();
                    verifier.do_play_card();
                    set_last_played_card(verifier.card_ptr);
                });
            } else {
                if (m_game->has_scenario(scenario_flags::judge)) {
                    throw game_error("ERROR_CANT_EQUIP_CARDS");
                }
                verifier.verify_equip_target();
                auto *target = std::get<target_player>(verifier.targets.front()).target;
                if (auto *card = target->find_equipped_card(verifier.card_ptr)) {
                    throw game_error("ERROR_DUPLICATED_CARD", card);
                }
                if (verifier.card_ptr->color == card_color_type::orange && m_game->m_cubes.size() < 3) {
                    throw game_error("ERROR_NOT_ENOUGH_CUBES");
                }
                prompt_then(verifier.check_prompt_equip(target), [this, card_ptr = verifier.card_ptr, target]{
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
                            if (m_game->has_expansion(card_expansion_type::armedanddangerous)) {
                                queue_request_add_cube(card_ptr);
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
        case pocket_type::scenario_card:
        case pocket_type::specials:
            if (m_game->is_disabled(verifier.card_ptr)) {
                throw game_error("ERROR_CARD_IS_DISABLED", verifier.card_ptr);
            }
            if (verifier.card_ptr->inactive) {
                throw game_error("ERROR_CARD_INACTIVE", verifier.card_ptr);
            }
            verifier.verify_modifiers();
            verifier.verify_card_targets();
            prompt_then(verifier.check_prompt(), [this, verifier]{
                verifier.play_modifiers();
                verifier.do_play_card();
                set_last_played_card(nullptr);
            });
            break;
        case pocket_type::hidden_deck:
            if (std::ranges::find(verifier.modifiers, card_modifier_type::shopchoice, &card::modifier) == verifier.modifiers.end()) {
                throw game_error("ERROR_INVALID_ACTION");
            }
            [[fallthrough]];
        case pocket_type::shop_selection: {
            int cost = verifier.card_ptr->buy_cost();
            for (card *c : verifier.modifiers) {
                if (m_game->is_disabled(c)) {
                    throw game_error("ERROR_CARD_IS_DISABLED", c);
                }
                switch (c->modifier) {
                case card_modifier_type::discount:
                    if (c->usages) throw game_error("ERROR_MAX_USAGES", c, 1);
                    --cost;
                    break;
                case card_modifier_type::shopchoice:
                    if (!c->effects.first_is(verifier.card_ptr->effects.front().type)) {
                        throw game_error("ERROR_INVALID_ACTION");
                    }
                    cost += c->buy_cost();
                    break;
                }
            }
            if (m_game->m_shop_selection.size() > 3) {
                cost = 0;
            }
            if (m_gold < cost) {
                throw game_error("ERROR_NOT_ENOUGH_GOLD");
            }
            if (verifier.card_ptr->color == card_color_type::brown) {
                verifier.verify_card_targets();
                prompt_then(verifier.check_prompt(), [=, this]{
                    verifier.play_modifiers();
                    add_gold(-cost);
                    verifier.do_play_card();
                    set_last_played_card(nullptr);
                    m_game->queue_action([&]{
                        while (m_game->m_shop_selection.size() < 3) {
                            m_game->draw_shop_card();
                        }
                    });
                });
            } else {
                if (m_game->has_scenario(scenario_flags::judge)) {
                    throw game_error("ERROR_CANT_EQUIP_CARDS");
                }
                verifier.verify_equip_target();
                auto *target = std::get<target_player>(verifier.targets.front()).target;
                if (card *card = target->find_equipped_card(verifier.card_ptr)) {
                    throw game_error("ERROR_DUPLICATED_CARD", card);
                }
                prompt_then(verifier.check_prompt_equip(target), [=, this]{
                    verifier.play_modifiers();
                    add_gold(-cost);
                    target->equip_card(verifier.card_ptr);
                    set_last_played_card(nullptr);
                    if (this == target) {
                        m_game->add_log("LOG_BOUGHT_EQUIP", verifier.card_ptr, this);
                    } else {
                        m_game->add_log("LOG_BOUGHT_EQUIP_TO", verifier.card_ptr, this, target);
                    }
                    m_game->call_event<event_type::on_effect_end>(this, verifier.card_ptr);
                    m_game->queue_action([&]{
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
        
        play_card_verify verifier{this, m_game->find_card(args.card_id), true, parse_target_id_vector(m_game, args.targets)};
        verifier.verify_card_targets();
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