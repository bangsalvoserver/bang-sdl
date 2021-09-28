#include "game.h"
#include "../../manager.h"

#include <iostream>

using namespace banggame;

template<game_action_type T, typename ... Ts>
void game_scene::add_action(Ts && ... args) {
    parent->add_message<client_message_type::game_action>(enums::enum_constant<T>{}, std::forward<Ts>(args) ...);
}

constexpr SDL_Point main_deck_position {380, 200};
constexpr SDL_Point discard_position {300, 200};

SDL_Point temp_table_card_position(int index) {
    return {300 + index * 30, 300};
}

SDL_Point player_hand_card_position(int player_index, int index) {
    return {10 + index * 30, 10 + player_index * 200};
}

SDL_Point player_table_card_position(int player_index, int index) {
    return {10 + index * 30, 100 + player_index * 200};
}

game_scene::game_scene(class game_manager *parent)
    : scene_base(parent) {}

static void scale_rect(SDL_Rect &rect, int new_w) {
    rect.h = new_w * rect.h / rect.w;
    rect.w = new_w;
}

static void render_card(sdl::renderer &renderer, card_view_location &c) {
    sdl::texture *tex = nullptr;
    if (c.flip_amt < 0.5f && c.texture_front) tex = &c.texture_front;
    else if (c.texture_back) tex = &c.texture_back;
    else return;
    
    SDL_Rect rect = tex->get_rect();
    rect.x = c.pos.x;
    rect.y = c.pos.y;
    scale_rect(rect, 70);

    float wscale = 2.f * std::abs(0.5f - c.flip_amt);
    rect.x += rect.w * (1.f - wscale) * 0.5f;
    rect.w *= wscale;
    SDL_RenderCopyEx(renderer.get(), tex->get_texture(renderer), nullptr, &rect, c.rotation, nullptr, SDL_FLIP_NONE);
}

void game_scene::render(sdl::renderer &renderer, int w, int h) {
    if (m_animations.empty()) {
        pop_update();
    } else {
        m_animations.front().tick();
        if (m_animations.front().done()) {
            m_animations.pop_front();
            pop_update();
        }
    }

    if (card_view_location::texture_back) {
        SDL_Rect rect = card_view_location::texture_back.get_rect();
        rect.x = main_deck_position.x;
        rect.y = main_deck_position.y;
        scale_rect(rect, 70);
        card_view_location::texture_back.render(renderer, rect);
    }

    for (int id : main_deck) {
        auto &c = m_cards.at(id);
        if (c.flip_amt < 1.f) {
            render_card(renderer, c);
            break;
        }
    }

    if (!discard_pile.empty()) {
        if (discard_pile.size() > 1) {
            render_card(renderer, m_cards.at(*(discard_pile.rbegin() + 1)));
        }
        render_card(renderer, m_cards.at(discard_pile.back()));
    }
    
    for (int id : temp_table) {
        render_card(renderer, m_cards.at(id));
    }
    
    for (auto &p : m_players) {
        for (int id : p.second.table) {
            render_card(renderer, m_cards.at(id));
        }
        for (int id : p.second.hand) {
            render_card(renderer, m_cards.at(id));
        }
    }
}

template<typename ... Ts>
static play_card_target make_player_target(Ts && ... args) {
    return play_card_target(enums::enum_constant<play_card_target_type::target_player>{},
        std::vector{target_player_id{std::forward<Ts>(args)} ... });
}

template<play_card_target_type E, typename ... Ts>
static play_card_target make_target(Ts && ... args) {
    return play_card_target(enums::enum_constant<E>{}, std::forward<Ts>(args) ...);
}

void game_scene::handle_event(const SDL_Event &event) {
    switch (event.type) {
    case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_d:
            add_action<game_action_type::pick_card>(card_pile_type::main_deck);
            break;
        case SDLK_p:
            add_action<game_action_type::pass_turn>();
            break;
        case SDLK_a:
            add_action<game_action_type::pick_card>(card_pile_type::player_hand, 0);
            break;
        case SDLK_s:
            add_action<game_action_type::pick_card>(card_pile_type::temp_table, 0);
            break;
        case SDLK_g:
            add_action<game_action_type::resolve>();
            break;
        case SDLK_e:
            for (int id : m_players.at(m_player_own_id).hand) {
                auto &c = m_cards.at(id);
                switch (c.card.color) {
                case card_color_type::blue: {
                    add_action<game_action_type::play_card>(id, std::vector{make_player_target(m_player_own_id)});
                    break;
                }
                case card_color_type::green:
                    add_action<game_action_type::play_card>(id);
                    break;
                default:
                    break;
                }
            }
            break;
        case SDLK_r: {
            auto find_card_named = [&](auto &&range, const std::string &name) {
                return std::ranges::find(range, name, [&](int id) { return m_cards.at(id).card.name; });
            };
            auto &table = m_players.at(m_player_own_id).table;
            if (auto it_dinamite = find_card_named(table, "Dinamite"); it_dinamite != table.end()) {
                add_action<game_action_type::pick_card>(card_pile_type::player_table, it_dinamite - table.begin());
            } else if (auto it_prigione = find_card_named(table, "Prigione"); it_prigione != table.end()) {
                add_action<game_action_type::pick_card>(card_pile_type::player_table, it_prigione - table.begin());
            }
            break;
        }
        case SDLK_ESCAPE:
            parent->add_message<client_message_type::lobby_leave>();
            parent->switch_scene<scene_type::lobby_list>();
            break;
        }
        break;
    }
}

void game_scene::handle_update(const game_update &update) {
    m_pending_updates.push_back(update);
}

void game_scene::pop_update() {
    if (!m_pending_updates.empty()) {
        const auto update = std::move(m_pending_updates.front());

        m_pending_updates.pop_front();
        enums::visit([this]<game_update_type E>(enums::enum_constant<E> tag, const auto & ... data) {
            handle_update(tag, data ...);
        }, update);
    }
}

void game_scene::add_chat_message(const lobby_chat_args &args) {
    m_messages.push_back(args);
}

void game_scene::handle_update(enums::enum_constant<game_update_type::game_notify>, game_notify_type args) {
    switch (args) {
    case game_notify_type::deck_shuffled: {
        int top_discard = discard_pile.back();
        discard_pile.resize(discard_pile.size() - 1);
        main_deck = std::move(discard_pile);
        discard_pile.clear();
        discard_pile.push_back(top_discard);
        for (int c : main_deck) {
            card_view_location &loc = m_cards.at(c);
            loc.card.known = false;
            loc.pile = card_pile_type::main_deck;
            loc.pos = main_deck_position;
            loc.flip_amt = 1.f;
        }
        std::cout << "Deck shuffled\n";
        break;
    }
    default:
        break;
    }
    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::move_card>, const move_card_update &args) {
    auto [it, inserted] = m_cards.try_emplace(args.card_id);
    card_move_animation anim;
    if (inserted) {
        it->second.pos = main_deck_position;
        it->second.flip_amt = 1.f;
        if (!it->second.texture_back) {
            it->second.texture_back = make_backface_texture();
        }
    } else {
        switch(it->second.pile) {
        case card_pile_type::player_hand: {
            auto pit = m_players.find(it->second.pile_value);
            int p_idx = std::distance(m_players.begin(), pit);
            auto &l = pit->second.hand;
            l.erase(std::ranges::find(l, args.card_id));
            for (size_t i=0; i<l.size(); ++i) {
                anim.add_move_card(m_cards.at(l[i]), player_hand_card_position(p_idx, i));
            }
            break;
        }
        case card_pile_type::player_table: {
            auto pit = m_players.find(it->second.pile_value);
            int p_idx = std::distance(m_players.begin(), pit);
            auto &l = pit->second.table;
            l.erase(std::ranges::find(l, args.card_id));
            for (size_t i=0; i<l.size(); ++i) {
                anim.add_move_card(m_cards.at(l[i]), player_table_card_position(p_idx, i));
            }
            break;
        }
        case card_pile_type::main_deck: {
            main_deck.erase(std::ranges::find(main_deck, args.card_id));
            break;
        }
        case card_pile_type::discard_pile: {
            main_deck.erase(std::ranges::find(discard_pile, args.card_id));
            break;
        }
        case card_pile_type::temp_table: {
            temp_table.erase(std::ranges::find(temp_table, args.card_id));
            for (size_t i=0; i<temp_table.size(); ++i) {
                anim.add_move_card(m_cards.at(temp_table[i]), temp_table_card_position(i));
            }
            break;
        }
        }
    }
    switch(args.destination) {
    case card_pile_type::player_hand: {
        auto pit = m_players.find(args.destination_value);
        int p_idx = std::distance(m_players.begin(), pit);
        auto &l = pit->second.hand;
        l.push_back(args.card_id);
        for (size_t i=0; i<l.size(); ++i) {
            anim.add_move_card(m_cards.at(l[i]), player_hand_card_position(p_idx, i));
        }
        break;
    }
    case card_pile_type::player_table: {
        auto pit = m_players.find(args.destination_value);
        int p_idx = std::distance(m_players.begin(), pit);
        auto &l = pit->second.table;
        l.push_back(args.card_id);
        for (size_t i=0; i<l.size(); ++i) {
            anim.add_move_card(m_cards.at(l[i]), player_table_card_position(p_idx, i));
        }
        break;
    }
    case card_pile_type::main_deck:
        main_deck.push_back(args.card_id);
        anim.add_move_card(it->second, main_deck_position);
        break;
    case card_pile_type::discard_pile:
        discard_pile.push_back(args.card_id);
        anim.add_move_card(it->second, discard_position);
        break;
    case card_pile_type::temp_table: {
        temp_table.push_back(args.card_id);
        for (size_t i=0; i<temp_table.size(); ++i) {
            anim.add_move_card(m_cards.at(temp_table[i]), temp_table_card_position(i));
        }
        break;
    }
    }
    it->second.pile = args.destination;
    it->second.pile_value = args.destination_value;
    if (inserted) {
        pop_update();
    } else {
        m_animations.emplace_back(30, std::move(anim));
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::show_card>, const show_card_update &args) {
    auto &c_view = m_cards.at(args.card_id);
    
    auto &c = c_view.card;

    if (!c.known) {
        c.known = true;
        c.name = args.name;
        c.image = args.image;
        c.suit = args.suit;
        c.value = args.value;
        c.color = args.color;
        c.targets.clear();

        for (const auto &t : args.targets) {
            c.targets.emplace_back(t.target, t.maxdistance);
        }

        c_view.texture_front = make_card_texture(c);

        m_animations.emplace_back(10, card_flip_animation{&c_view, false});
    } else {
        pop_update();
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::hide_card>, const hide_card_update &args) {
    auto &c_view = m_cards.at(args.card_id);
    if (c_view.card.known) {
        c_view.card.known = false;
        m_animations.emplace_back(10, card_flip_animation{&c_view, true});
    } else {
        pop_update();
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::tap_card>, const tap_card_update &args) {
    auto &c_view = m_cards.at(args.card_id);
    if (c_view.card.active != args.active) {
        c_view.card.active = args.active;
        m_animations.emplace_back(10, card_tap_animation{&c_view, !args.active});
    } else {
        pop_update();
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_own_id>, const player_own_id_update &args) {
    m_player_own_id = args.player_id;

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_hp>, const player_hp_update &args) {
    auto &p = m_players.at(args.player_id).player;
    if (p.hp == 0) {
        pop_update();
    } else {
        m_animations.emplace_back(20, player_hp_animation{&p, p.hp, args.hp});
    }
    p.hp = args.hp;
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_character>, const player_character_update &args) {
    auto &p = m_players[args.player_id].player;
    p.name = args.name;
    p.image = args.image;
    p.character_id = args.card_id;
    p.target = args.target;

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_show_role>, const player_show_role_update &args) {
    m_players.at(args.player_id).player.role = args.role;
    
    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::switch_turn>, const switch_turn_update &args) {
    m_playing_id = args.player_id;

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::response_handle>, const response_handle_update &args) {
    m_current_response.type = args.type;
    m_current_response.origin_id = args.origin_id;
    m_current_response.target_id = args.target_id;

    using namespace enums::stream_operators;

    if (args.target_id == m_player_own_id) {
        std::cout << "Do response: " << args.type << '\n';
    }

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::response_done>) {
    m_current_response.type = response_type::none;
    m_current_response.origin_id = 0;

    std::cout << "Response done\n";

    pop_update();
}