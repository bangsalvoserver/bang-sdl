#include "manager.h"

#include <iostream>
#include <stdexcept>

#include "common/net_enums.h"
#include "common/binary_serial.h"

using namespace banggame;
using namespace enums::flag_operators;
using namespace std::string_literals;

game_manager::game_manager()
    : all_cards(make_all_cards()) {}

void game_manager::parse_message(const sdlnet::ip_address &addr, const std::vector<std::byte> &bytes) {
    try {
        auto msg = binary::deserialize<client_message>(bytes);
        enums::visit_indexed([&](auto enum_const, auto && ... args) {
            return handle_message(enum_const, addr, std::forward<decltype(args)>(args) ...);
        }, msg);
    } catch (const game_error &e) {
        send_message<server_message_type::game_update>(addr, enums::enum_constant<game_update_type::game_error>(), e);
    } catch (const binary::read_error &e) {
        print_error(addr.ip_string() + ": Deserialization Error: "s + e.what());
    } catch (const std::exception &e) {
        print_error(addr.ip_string() + ": Error: "s + e.what());
    }
}

void game_manager::tick() {
    for (auto &l : m_lobbies) {
        if (l.state == lobby_state::playing) {
            l.game.tick();
        }
        l.send_updates(*this);
    }
}

int game_manager::pending_messages() {
    return m_out_queue.size();
}

server_message_pair game_manager::pop_message() {
    auto msg = std::move(m_out_queue.front());
    m_out_queue.pop_front();
    return msg;
}

game_user *game_manager::find_user(const sdlnet::ip_address &addr) {
    if (auto it = users.find(addr); it != users.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

std::list<lobby>::iterator game_manager::find_lobby(const game_user *u) {
    return std::ranges::find_if(m_lobbies, [&](const lobby &l) {
        return std::ranges::find(l.users, u, &lobby_user::user) != l.users.end();
    });
}

void game_manager::handle_message(MESSAGE_TAG(connect), const sdlnet::ip_address &addr, const connect_args &args) {
    if (users.try_emplace(addr, addr, args.user_name, args.profile_image).second) {
        send_message<server_message_type::client_accepted>(addr);
    }
}

lobby_data game_manager::make_lobby_data(const lobby &l) {
    lobby_data obj;
    obj.lobby_id = l.id;
    obj.name = l.name;
    obj.num_players = l.users.size();
    obj.state = l.state;
    return obj;
}

void game_manager::send_lobby_update(const lobby &l) {
    auto msg = make_message<server_message_type::lobby_update>(make_lobby_data(l));
    for (const auto &addr : users | std::views::keys) {
        m_out_queue.emplace_back(addr, msg);
    }
}

void game_manager::handle_message(MESSAGE_TAG(lobby_list), const sdlnet::ip_address &addr) {
    std::vector<lobby_data> vec;
    for (const auto &lobby : m_lobbies) {
        vec.push_back(make_lobby_data(lobby));
    }
    send_message<server_message_type::lobby_list>(addr, std::move(vec));
}

void game_manager::handle_message(MESSAGE_TAG(lobby_make), const sdlnet::ip_address &addr, const lobby_info &value) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it != m_lobbies.end()) {
        throw game_error("ERROR_PLAYER_IN_LOBBY");
    }

    lobby new_lobby;
    new_lobby.users.emplace_back(u, nullptr);

    new_lobby.owner = u;
    new_lobby.name = value.name;
    new_lobby.state = lobby_state::waiting;
    new_lobby.expansions = value.expansions;
    send_lobby_update(m_lobbies.emplace_back(std::move(new_lobby)));

    send_message<server_message_type::lobby_entered>(addr, value, u->id, u->id);
}

void game_manager::handle_message(MESSAGE_TAG(lobby_edit), const sdlnet::ip_address &addr, const lobby_info &args) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it == m_lobbies.end()) {
        throw game_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    if (it->owner != u) {
        throw game_error("ERROR_PLAYER_NOT_LOBBY_OWNER");
    }

    if (it->state != lobby_state::waiting) {
        throw game_error("ERROR_LOBBY_NOT_WAITING");
    }

    it->name = args.name;
    it->expansions = args.expansions;
    for (auto &p : it->users) {
        if (p.user != u) {
            send_message<server_message_type::lobby_edited>(p.user->addr, args);
        }
    }
}

void game_manager::handle_message(MESSAGE_TAG(lobby_join), const sdlnet::ip_address &addr, const lobby_join_args &value) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = std::ranges::find(m_lobbies, value.lobby_id, &lobby::id);
    if (it == m_lobbies.end()) {
        throw game_error("Invalid Lobby ID"_nonloc);
    }

    if (it->users.size() < lobby_max_players) {
        auto &new_user = it->users.emplace_back(u, nullptr);
        send_lobby_update(*it);

        for (auto &p : it->users) {
            if (p.user == u) {
                send_message<server_message_type::lobby_entered>(p.user->addr, lobby_info{it->name, it->expansions}, u->id, it->owner->id);
            } else {
                send_message<server_message_type::lobby_joined>(p.user->addr, u->id, u->name, u->profile_image);
            }
        }
        if (it->state != lobby_state::waiting) {
            auto dc_player = std::ranges::find_if(it->game.m_players, [&](const player &p) {
                return std::ranges::find(it->users, &p, &lobby_user::controlling) == it->users.end();
            });
            if (dc_player != it->game.m_players.end()) {
                new_user.controlling = &*dc_player;
            }

            std::vector<lobby_player_data> vec;
            for (const auto &l_u : it->users) {
                vec.emplace_back(l_u.user->id, l_u.user->name, l_u.user->profile_image);
            }
            send_message<server_message_type::lobby_players>(addr, std::move(vec));

            send_message<server_message_type::game_started>(addr, it->game.m_options.expansions);
            for (const player &p : it->game.m_players) {
                auto u_it = std::ranges::find(it->users, &p, &lobby_user::controlling);
                send_message<server_message_type::game_update>(addr, enums::enum_constant<game_update_type::player_add>{},
                    p.id, u_it == it->users.end() ? 0 : u_it->user->id);
            }
            for (const auto &msg : it->game.get_game_state_updates(new_user.controlling)) {
                send_message<server_message_type::game_update>(addr, msg);
            }

            if (new_user.controlling) {
                broadcast_message<server_message_type::game_update>(*it,
                    enums::enum_constant<game_update_type::player_add>{}, new_user.controlling->id, u->id);
            }
        }
    }
}

void game_manager::handle_message(MESSAGE_TAG(lobby_players), const sdlnet::ip_address &addr) {
    auto it = find_lobby(find_user(addr));
    if (it == m_lobbies.end()) {
        throw game_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    std::vector<lobby_player_data> vec;
    for (const auto &u : it->users) {
        vec.emplace_back(u.user->id, u.user->name, u.user->profile_image);
    }

    send_message<server_message_type::lobby_players>(addr, std::move(vec));
}

void game_manager::client_disconnected(const sdlnet::ip_address &addr) {
    handle_message(MESSAGE_TAG(lobby_leave){}, addr);
    users.erase(addr);
}

void game_manager::handle_message(MESSAGE_TAG(lobby_leave), const sdlnet::ip_address &addr) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto lobby_it = find_lobby(u);
    if (lobby_it != m_lobbies.end()) {
        auto player_it = std::ranges::find(lobby_it->users, u, &lobby_user::user);
        broadcast_message<server_message_type::lobby_left>(*lobby_it, player_it->user->id);
        lobby_it->users.erase(player_it);
        send_lobby_update(*lobby_it);
        
        if (lobby_it->users.empty()) {
            m_lobbies.erase(lobby_it);
        } else if (lobby_it->state == lobby_state::waiting && u == lobby_it->owner) {
            for (auto &u : lobby_it->users) {
                broadcast_message<server_message_type::lobby_left>(*lobby_it, u.user->id);
            }
            m_lobbies.erase(lobby_it);
        }
    }
}

void game_manager::handle_message(MESSAGE_TAG(lobby_chat), const sdlnet::ip_address &addr, const lobby_chat_client_args &value) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it == m_lobbies.end()) {
        throw game_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    broadcast_message<server_message_type::lobby_chat>(*it, u->id, value.message);
}

void game_manager::handle_message(MESSAGE_TAG(game_start), const sdlnet::ip_address &addr) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it == m_lobbies.end()) {
        throw game_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    if (u != it->owner) {
        throw game_error("ERROR_PLAYER_NOT_LOBBY_OWNER");
    }

    if (it->state == lobby_state::playing) {
        throw game_error("ERROR_LOBBY_NOT_WAITING");
    }

    it->state = lobby_state::playing;

    broadcast_message<server_message_type::game_started>(*it, it->expansions | card_expansion_type::base);

    it->start_game(all_cards);
}

void game_manager::handle_message(MESSAGE_TAG(game_action), const sdlnet::ip_address &addr, const game_action &value) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it == m_lobbies.end()) {
        throw game_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    if (it->state != lobby_state::playing) {
        throw game_error("ERROR_LOBBY_NOT_PLAYING");
    }

    auto *controlling = std::ranges::find(it->users, u, &lobby_user::user)->controlling;

    enums::visit_indexed([&]<game_action_type T>(enums::enum_constant<T> tag, auto && ... args) {
        it->game.handle_action(tag, controlling, std::forward<decltype(args)>(args) ...);
    }, value);
}

void lobby::send_updates(game_manager &mgr) {
    while (!game.m_updates.empty()) {
        const auto &data = game.m_updates.front();
        if (data.first) {
            const auto &addr = std::ranges::find(users, data.first, &lobby_user::controlling)->user->addr;
            mgr.send_message<server_message_type::game_update>(addr, data.second);
        } else {
            mgr.broadcast_message<server_message_type::game_update>(*this, data.second);
        }
        if (data.second.is(game_update_type::game_over)) {
            state = lobby_state::finished;
        }
        game.m_updates.pop_front();
    }
}

void lobby::start_game(const banggame::all_cards_t &all_cards) {
    game = {};

    game_options opts;
    opts.expansions = expansions | banggame::card_expansion_type::base;
    
    std::vector<player *> ids;
    game.m_players.reserve(users.size());
    for (int i = 0; i < users.size(); ++i) {
        ids.push_back(&game.m_players.emplace_back(&game));
    }
    std::ranges::shuffle(ids, game.rng);

    auto it = users.begin();
    for (player *p : ids) {
        it->controlling = p;
        game.add_public_update<game_update_type::player_add>(p->id, it->user->id);
        ++it;
    }

    game.start_game(opts, all_cards);
}