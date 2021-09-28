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

    if (card_view::texture_back) {
        SDL_Rect rect = card_view::texture_back.get_rect();
        rect.x = main_deck_position.x;
        rect.y = main_deck_position.y;
        scale_rect(rect, 70);
        card_view::texture_back.render(renderer, rect);
    }

    for (int id : main_deck) {
        auto &c = get_card(id);
        if (c.flip_amt > 0.f) {
            c.render(renderer);
            break;
        }
    }

    for (int id : discard_pile | std::views::reverse | std::views::take(2) | std::views::reverse) {
        get_card(id).render(renderer);
    }
    
    for (int id : temp_table) {
        get_card(id).render(renderer);
    }
    
    for (auto &p : m_players) {
        for (int id : p.second.table) {
            get_card(id).render(renderer);
        }
        for (int id : p.second.hand) {
            get_card(id).render(renderer);
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
            if (auto &hand = get_player(m_player_own_id).hand; !hand.empty()) {
                add_action<game_action_type::pick_card>(card_pile_type::player_hand, hand.front());
            }
            break;
        case SDLK_s:
            if (!temp_table.empty()) {
                add_action<game_action_type::pick_card>(card_pile_type::temp_table, temp_table.front());
            }
            break;
        case SDLK_g:
            add_action<game_action_type::resolve>();
            break;
        case SDLK_e:
            for (int id : get_player(m_player_own_id).hand) {
                auto &c = get_card(id);
                switch (c.color) {
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
            auto &table = get_player(m_player_own_id).table;
            auto check_for = [&](const std::string &name) {
                auto it = std::ranges::find(table, name, [&](int id) { return get_card(id).name; });
                if (it != table.end()) {
                    add_action<game_action_type::pick_card>(card_pile_type::player_table, *it);
                }
            };
            check_for("Dinamite");
            check_for("Prigione");
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
    try {
        if (!m_pending_updates.empty()) {
            const auto update = std::move(m_pending_updates.front());

            m_pending_updates.pop_front();
            enums::visit([this]<game_update_type E>(enums::enum_constant<E> tag, const auto & ... data) {
                handle_update(tag, data ...);
            }, update);
        }
    } catch (const std::exception &error) {
        std::cerr << "Errore: " << error.what() << '\n';
        parent->disconnect();
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
            card_view &loc = get_card(c);
            loc.known = false;
            loc.pile = card_pile_type::main_deck;
            loc.pos = main_deck_position;
            loc.flip_amt = 0.f;
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
    auto [card_it, inserted] = m_cards.try_emplace(args.card_id);
    card_move_animation anim;
    if (inserted) {
        card_it->second.pos = main_deck_position;
        if (!card_it->second.texture_back) {
            card_it->second.texture_back = make_backface_texture();
        }
    } else {
        switch(card_it->second.pile) {
        case card_pile_type::player_hand: {
            auto player_it = m_players.find(card_it->second.player_id);
            if (player_it != m_players.end()) {
                int p_idx = std::distance(m_players.begin(), player_it);
                auto &l = player_it->second.hand;
                l.erase(std::ranges::find(l, args.card_id));
                for (size_t i=0; i<l.size(); ++i) {
                    anim.add_move_card(get_card(l[i]), player_hand_card_position(p_idx, i));
                }
            }
            break;
        }
        case card_pile_type::player_table: {
            auto player_it = m_players.find(card_it->second.player_id);
            if (player_it != m_players.end()) {
                int p_idx = std::distance(m_players.begin(), player_it);
                auto &l = player_it->second.table;
                l.erase(std::ranges::find(l, args.card_id));
                for (size_t i=0; i<l.size(); ++i) {
                    anim.add_move_card(get_card(l[i]), player_table_card_position(p_idx, i));
                }
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
                anim.add_move_card(get_card(temp_table[i]), temp_table_card_position(i));
            }
            break;
        }
        }
    }
    switch(args.pile) {
    case card_pile_type::player_hand: {
        auto player_it = m_players.find(args.player_id);
        if (player_it != m_players.end()) {
            int p_idx = std::distance(m_players.begin(), player_it);
            auto &l = player_it->second.hand;
            l.push_back(args.card_id);
            for (size_t i=0; i<l.size(); ++i) {
                anim.add_move_card(get_card(l[i]), player_hand_card_position(p_idx, i));
            }
        }
        break;
    }
    case card_pile_type::player_table: {
        auto player_it = m_players.find(args.player_id);
        if (player_it != m_players.end()) {
            int p_idx = std::distance(m_players.begin(), player_it);
            auto &l = player_it->second.table;
            l.push_back(args.card_id);
            for (size_t i=0; i<l.size(); ++i) {
                anim.add_move_card(get_card(l[i]), player_table_card_position(p_idx, i));
            }
        }
        break;
    }
    case card_pile_type::main_deck:
        main_deck.push_back(args.card_id);
        anim.add_move_card(card_it->second, main_deck_position);
        break;
    case card_pile_type::discard_pile:
        discard_pile.push_back(args.card_id);
        anim.add_move_card(card_it->second, discard_position);
        break;
    case card_pile_type::temp_table: {
        temp_table.push_back(args.card_id);
        for (size_t i=0; i<temp_table.size(); ++i) {
            anim.add_move_card(get_card(temp_table[i]), temp_table_card_position(i));
        }
        break;
    }
    }
    card_it->second.pile = args.pile;
    card_it->second.player_id = args.player_id;
    if (inserted) {
        pop_update();
    } else {
        m_animations.emplace_back(30, std::move(anim));
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::show_card>, const show_card_update &args) {
    auto &c_view = get_card(args.card_id);

    if (!c_view.known) {
        c_view.known = true;
        c_view.name = args.name;
        c_view.image = args.image;
        c_view.suit = args.suit;
        c_view.value = args.value;
        c_view.color = args.color;
        c_view.targets = args.targets;

        c_view.texture_front = make_card_texture(c_view);

        m_animations.emplace_back(10, card_flip_animation{&c_view, false});
    } else {
        pop_update();
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::hide_card>, const hide_card_update &args) {
    auto &c_view = get_card(args.card_id);
    if (c_view.known) {
        c_view.known = false;
        m_animations.emplace_back(10, card_flip_animation{&c_view, true});
    } else {
        pop_update();
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::tap_card>, const tap_card_update &args) {
    auto &c_view = get_card(args.card_id);
    if (c_view.inactive != args.inactive) {
        c_view.inactive = args.inactive;
        m_animations.emplace_back(10, card_tap_animation{&c_view, args.inactive});
    } else {
        pop_update();
    }
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_own_id>, const player_own_id_update &args) {
    m_player_own_id = args.player_id;

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_hp>, const player_hp_update &args) {
    auto &p = get_player(args.player_id);
    if (p.hp == 0) {
        pop_update();
    } else {
        m_animations.emplace_back(20, player_hp_animation{&p, p.hp, args.hp});
    }
    p.hp = args.hp;
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_character>, const player_character_update &args) {
    auto &p = m_players[args.player_id];
    p.name = args.name;
    p.image = args.image;
    p.character_id = args.card_id;
    p.target = args.target;

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_show_role>, const player_show_role_update &args) {
    get_player(args.player_id).role = args.role;
    
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