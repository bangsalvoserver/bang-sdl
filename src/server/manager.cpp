#include "manager.h"

#include <iostream>
#include <stdexcept>

#include "common/net_enums.h"

using namespace banggame;
using namespace enums::flag_operators;

void game_manager::parse_message(const sdlnet::ip_address &addr, const std::string &str) {
    constexpr auto lut = []<client_message_type ... Es>(enums::enum_sequence<Es...>) {
        return std::array{ +[](game_manager &mgr, const sdlnet::ip_address &addr, const Json::Value &value) {
            constexpr client_message_type E = Es;
            if constexpr (enums::has_type<E>) {
                mgr.handle_message(enums::enum_constant<E>{}, addr, json::deserialize<enums::enum_type_t<E>>(value));
            } else {
                mgr.handle_message(enums::enum_constant<E>{}, addr);
            }
        } ... };
    }(enums::make_enum_sequence<client_message_type>());

    std::stringstream ss(str);

    try {
        Json::Value json_value;
        ss >> json_value;

        auto msg = enums::from_string<client_message_type>(json_value["type"].asString());
        if (msg != enums::invalid_enum_v<client_message_type>) {
            lut[enums::indexof(msg)](*this, addr, json_value["value"]);
        }
    } catch (const game_error &e) {
        send_message<server_message_type::game_error>(addr, e.what());
    } catch (const Json::Exception &e) {
        std::cerr << addr.ip_string() << ": Errore Json (" << e.what() << ")\n";
    } catch (const std::exception &e) {
        std::cerr << addr.ip_string() << ": Errore (" << e.what() << ")\n";
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

server_message game_manager::pop_message() {
    auto msg = std::move(m_out_queue.front());
    m_out_queue.pop_front();
    return msg;
}

user *game_manager::find_user(const sdlnet::ip_address &addr) {
    if (auto it = users.find(addr); it != users.end()) {
        return &it->second;
    } else {
        return nullptr;
    }
}

std::list<lobby>::iterator game_manager::find_lobby(const user *u) {
    return std::ranges::find_if(m_lobbies, [&](const lobby &l) {
        return std::ranges::find(l.users, u, &lobby_user::user) != l.users.end();
    });
}

void game_manager::handle_message(enums::enum_constant<client_message_type::connect>, const sdlnet::ip_address &addr, const connect_args &args) {
    if (users.try_emplace(addr, addr, args.user_name).second) {
        send_message<server_message_type::client_accepted>(addr);
    }
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_list>, const sdlnet::ip_address &addr) {
    std::vector<lobby_data> vec;
    for (const auto &lobby : m_lobbies) {
        lobby_data obj;
        obj.lobby_id = lobby.id;
        obj.name = lobby.name;
        obj.num_players = lobby.users.size();
        obj.state = lobby.state;
        vec.push_back(obj);
    }
    send_message<server_message_type::lobby_list>(addr, std::move(vec));
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_make>, const sdlnet::ip_address &addr, const lobby_info &value) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it != m_lobbies.end()) {
        throw game_error("Giocatore gia' in una lobby");
    }

    lobby new_lobby;
    new_lobby.users.emplace_back(u, nullptr);

    new_lobby.owner = u;
    new_lobby.name = value.name;
    new_lobby.state = lobby_state::waiting;
    new_lobby.expansions = value.expansions;
    m_lobbies.push_back(new_lobby);

    send_message<server_message_type::lobby_entered>(addr, value, u->id, u->id);
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_edit>, const sdlnet::ip_address &addr, const lobby_info &args) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it == m_lobbies.end()) {
        throw game_error("Giocatore non in una lobby");
    }

    if (it->owner != u) {
        throw game_error("Non proprietario della lobby");
    }

    if (it->state != lobby_state::waiting) {
        throw game_error("Lobby non in attesa");
    }

    it->name = args.name;
    it->expansions = args.expansions;
    for (auto &p : it->users) {
        if (p.user != u) {
            send_message<server_message_type::lobby_edited>(p.user->addr, args);
        }
    }
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_join>, const sdlnet::ip_address &addr, const lobby_join_args &value) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = std::ranges::find(m_lobbies, value.lobby_id, &lobby::id);
    if (it == m_lobbies.end()) {
        throw game_error("Id Lobby non valido");
    }

    if (it->state != lobby_state::waiting) {
        throw game_error("Lobby non in attesa");
    }

    if (it->users.size() < lobby_max_players) {
        it->users.emplace_back(u, nullptr);

        for (auto &p : it->users) {
            if (p.user == u) {
                send_message<server_message_type::lobby_entered>(p.user->addr, lobby_info{it->name, it->expansions}, u->id, it->owner->id);
            } else {
                send_message<server_message_type::lobby_joined>(p.user->addr, u->id, u->name);
            }
        }
    }
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_players>, const sdlnet::ip_address &addr) {
    auto it = find_lobby(find_user(addr));
    if (it == m_lobbies.end()) {
        throw game_error("Giocatore non in una lobby");
    }

    std::vector<lobby_player_data> vec;
    for (const auto &u : it->users) {
        vec.emplace_back(u.user->id, u.user->name);
    }

    send_message<server_message_type::lobby_players>(addr, std::move(vec));
}

void game_manager::client_disconnected(const sdlnet::ip_address &addr) {
    handle_message(enums::enum_constant<client_message_type::lobby_leave>{}, addr);
    users.erase(addr);
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_leave>, const sdlnet::ip_address &addr) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto lobby_it = find_lobby(u);
    if (lobby_it != m_lobbies.end()) {
        auto player_it = std::ranges::find(lobby_it->users, u, &lobby_user::user);
        broadcast_message<server_message_type::lobby_left>(*lobby_it, player_it->user->id);
        lobby_it->users.erase(player_it);
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

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_chat>, const sdlnet::ip_address &addr, const lobby_chat_client_args &value) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it == m_lobbies.end()) {
        throw game_error("Giocatore non in una lobby");
    }

    broadcast_message<server_message_type::lobby_chat>(*it, u->id, value.message);
}

void game_manager::handle_message(enums::enum_constant<client_message_type::game_start>, const sdlnet::ip_address &addr) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it == m_lobbies.end()) {
        throw game_error("Giocatore non in una lobby");
    }

    if (u != it->owner) {
        throw game_error("Non proprietario della lobby");
    }

    if (it->state == lobby_state::playing) {
        throw game_error("Lobby non in attesa");
    }

    it->state = lobby_state::playing;

    broadcast_message<server_message_type::game_started>(*it);

    it->start_game();
}

void game_manager::handle_message(enums::enum_constant<client_message_type::game_action>, const sdlnet::ip_address &addr, const game_action &value) {
    auto *u = find_user(addr);
    if (!u) {
        return;
    }

    auto it = find_lobby(u);
    if (it == m_lobbies.end()) {
        throw game_error("Giocatore non in una lobby");
    }

    if (it->state != lobby_state::playing) {
        throw game_error("Lobby non in gioco");
    }

    auto *controlling = std::ranges::find(it->users, u, &lobby_user::user)->controlling;

    enums::visit_indexed([&]<game_action_type T>(enums::enum_constant<T> tag, auto && ... args) {
        it->game.handle_action(tag, controlling, std::forward<decltype(args)>(args) ...);
    }, value);
}

void lobby::send_updates(game_manager &mgr) {
    while (!game.m_updates.empty()) {
        const auto &data = game.m_updates.front();
        if (data.second.is(game_update_type::game_over)) {
            state = lobby_state::finished;
        }
        const auto &addr = std::ranges::find(users, data.first, &lobby_user::controlling)->user->addr;
        mgr.send_message<server_message_type::game_update>(addr, data.second);
        game.m_updates.pop_front();
    }
}

void lobby::start_game() {
    game = {};

    game_options opts;
    opts.nplayers = users.size();
    opts.expansions = expansions | banggame::card_expansion_type::base;
    
    for (int i = 0; i < opts.nplayers; ++i) {
        game.m_players.emplace_back(&game);
    }

    auto it = users.begin();
    for (auto &p : game.m_players) {
        it->controlling = &p;
        game.add_public_update<game_update_type::player_add>(p.id, it->user->id);
        ++it;
    }

    game.start_game(opts);
}