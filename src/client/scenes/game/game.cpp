#include "game.h"
#include "../../manager.h"

#include <iostream>
#include <numbers>

using namespace banggame;

template<game_action_type T, typename ... Ts>
void game_scene::add_action(Ts && ... args) {
    parent->add_message<client_message_type::game_action>(enums::enum_constant<T>{}, std::forward<Ts>(args) ...);
}

game_scene::game_scene(class game_manager *parent)
    : scene_base(parent) {}

void game_scene::resize(int width, int height) {
    scene_base::resize(width, height);
    
    main_deck_card.pos = SDL_Point{m_width / 2, m_height / 2};

    discard_pile.pos = main_deck_card.pos;
    discard_pile.pos.x -= 80;
    
    temp_table.pos = SDL_Point{
        (main_deck_card.pos.x + discard_pile.pos.x) / 2,
        main_deck_card.pos.y + 100};
}

void game_scene::render(sdl::renderer &renderer) {
    if (m_animations.empty()) {
        pop_update();
    } else {
        m_animations.front().tick();
        if (m_animations.front().done()) {
            m_animations.pop_front();
            pop_update();
        }
    }
    
    if (!m_player_own_id) return;

    main_deck_card.render(renderer);

    for (int id : main_deck) {
        auto &c = get_card(id);
        if (c.flip_amt > 0.f) {
            c.render(renderer);
            break;
        }
    }

    for (auto &p : m_players) {
        for (int id : p.second.table) {
            get_card(id).render(renderer);
        }
        for (int id : p.second.hand) {
            get_card(id).render(renderer);
        }
    }

    for (int id : discard_pile | std::views::reverse | std::views::take(2) | std::views::reverse) {
        get_card(id).render(renderer);
    }

    for (int id : temp_table) {
        get_card(id).render(renderer);
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
            loc.pile = &main_deck;
            loc.pos = main_deck_card.pos;
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
        card_it->second.pos = main_deck_card.pos;
        card_it->second.pile = &main_deck;
        if (!card_it->second.texture_back) {
            card_it->second.texture_back = make_backface_texture();
        }
    } else if (auto *pile = card_it->second.pile) {
        pile->erase_card(args.card_id);
        if (pile->xoffset) {
            for (int card_id : *pile) {
                anim.add_move_card(get_card(card_id), pile->get_position(card_id));
            }
        }
    }
    card_pile_view *pile = nullptr;
    switch(args.pile) {
    case card_pile_type::player_hand:   pile = &get_player(args.player_id).hand; break;
    case card_pile_type::player_table:  pile = &get_player(args.player_id).table; break;
    case card_pile_type::main_deck:     pile = &main_deck; break;
    case card_pile_type::discard_pile:  pile = &discard_pile; break;
    case card_pile_type::temp_table:    pile = &temp_table; break;
    }
    if (card_it->second.pile = pile) {
        pile->push_back(args.card_id);
        if (pile->xoffset) {
            for (int card_id : *pile) {
                anim.add_move_card(get_card(card_id), pile->get_position(card_id));
            }
        } else {
            anim.add_move_card(get_card(args.card_id), pile->get_position(args.card_id));
        }
    }
    if (inserted) {
        pop_update();
    } else {
        m_animations.emplace_back(20, std::move(anim));
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

void game_scene::handle_update(enums::enum_constant<game_update_type::player_own_id>, const player_id_update &args) {
    auto own_player = m_players.find(m_player_own_id = args.player_id);

    SDL_Point pos{m_width / 2, m_height - 120};
    own_player->second.set_position(pos, true);

    int xradius = (m_width - 150) - (m_width / 2);
    int yradius = pos.y - (m_height / 2);

    auto it = own_player;
    double angle = std::numbers::pi * 1.5f;
    for(;;) {
        if (++it == m_players.end()) it = m_players.begin();
        if (it == own_player) break;
        angle -= std::numbers::pi * 2.f / m_players.size();
        it->second.set_position(SDL_Point{
            int(m_width / 2 + std::cos(angle) * xradius),
            int(m_height / 2 - std::sin(angle) * yradius)
        });
    }

    pop_update();
}

void game_scene::handle_update(enums::enum_constant<game_update_type::player_add>, const player_id_update &args) {
    m_players.try_emplace(args.player_id);

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
    auto &p = get_player(args.player_id);
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