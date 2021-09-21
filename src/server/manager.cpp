#include "manager.h"

#include <iostream>
#include <stdexcept>

using namespace banggame;

void game_manager::parse_message(const sdlnet::ip_address &addr, const std::string &str) {
    std::stringstream ss(str);

    try {
        Json::Value json_value;
        ss >> json_value;

        auto msg = enums::from_string<client_message_type>(json_value["type"].asString());
        if (msg == enums::invalid_enum_v<client_message_type>) {
            throw std::runtime_error("Tipo messaggio non valido");
        }

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

        lut[enums::indexof(msg)](*this, addr, json_value["value"]);
    } catch (const std::exception &e) {
        std::cerr << addr.ip_string() << ": Errore (" << e.what() << ")\n";
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

std::list<lobby>::iterator game_manager::find_lobby(const sdlnet::ip_address &addr) {
    return std::ranges::find_if(m_lobbies, [&](const lobby &l) {
        using pair_type = decltype(l.users)::value_type;
        return std::ranges::find(l.users, addr, &pair_type::first) != l.users.end();
    });
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_list>, const sdlnet::ip_address &addr) {
    std::vector<lobby_data> vec;
    for (const auto &lobby : m_lobbies) {
        lobby_data obj;
        obj.id = lobby.id;
        obj.name = lobby.name;
        obj.num_players = lobby.users.size();
        obj.max_players = lobby.maxplayers;
        obj.state = lobby.state;
        vec.push_back(obj);
    }
    send_message<server_message_type::lobby_list>(addr, std::move(vec));
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_make>, const sdlnet::ip_address &addr, const lobby_make_args &value) {
    auto it = find_lobby(addr);
    if (it != m_lobbies.end()) {
        throw std::runtime_error("Giocatore gia' in una lobby");
    }

    lobby new_lobby;
    auto &u = new_lobby.users.emplace(addr, user()).first->second;
    u.name = value.player_name;

    new_lobby.owner = addr;
    new_lobby.name = value.lobby_name;
    new_lobby.state = lobby_state::waiting;
    new_lobby.maxplayers = value.max_players;
    m_lobbies.push_back(new_lobby);

    send_message<server_message_type::lobby_joined>(addr, u.id, new_lobby.name);
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_join>, const sdlnet::ip_address &addr, const lobby_join_args &value) {
    auto it = std::ranges::find(m_lobbies, value.lobby_id, &lobby::id);
    if (it == m_lobbies.end()) {
        throw std::runtime_error("Id Lobby non valido");
    }

    auto &u = it->users.emplace(addr, user()).first->second;
    u.name = value.player_name;

    send_message<server_message_type::lobby_joined>(addr, u.id, it->name);
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_players>, const sdlnet::ip_address &addr) {
    auto it = find_lobby(addr);
    if (it == m_lobbies.end()) {
        throw std::runtime_error("Giocatore non in una lobby");
    }

    std::vector<lobby_player_data> vec;
    for (const auto &u : it->users | std::views::values) {
        vec.emplace_back(u.id, u.name);
    }

    send_message<server_message_type::lobby_players>(addr, std::move(vec));
}

void game_manager::client_disconnected(const sdlnet::ip_address &addr) {
    auto lobby_it = find_lobby(addr);
    if (lobby_it != m_lobbies.end()) {
        if (addr == lobby_it->owner) {
            broadcast_message<server_message_type::lobby_deleted>(*lobby_it, lobby_it->id);
            m_lobbies.erase(lobby_it);
        } else {
            auto player_it = lobby_it->users.find(addr);
            broadcast_message<server_message_type::lobby_left>(*lobby_it, player_it->second.id);
            lobby_it->users.erase(player_it);
        }
    }
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_leave>, const sdlnet::ip_address &addr) {
    client_disconnected(addr);
}

void game_manager::handle_message(enums::enum_constant<client_message_type::lobby_chat>, const sdlnet::ip_address &addr, const lobby_chat_client_args &value) {
    auto it = find_lobby(addr);
    if (it == m_lobbies.end()) {
        throw std::runtime_error("Giocatore non in una lobby");
    }

    const auto &u = it->users.at(addr);
    broadcast_message<server_message_type::lobby_chat>(*it, u.name, value.message);
}

void game_manager::handle_message(enums::enum_constant<client_message_type::game_start>, const sdlnet::ip_address &addr) {
    auto it = find_lobby(addr);
    if (it == m_lobbies.end()) {
        throw std::runtime_error("Giocatore non in una lobby");
    }

    if (addr != it->owner) {
        throw std::runtime_error("Non proprietario della lobby");
    }

    if (it->state != lobby_state::waiting) {
        throw std::runtime_error("Lobby non in attesa");
    }

    it->state = lobby_state::playing;

    it->start_game();
    it->send_updates(*this);
}

void game_manager::handle_message(enums::enum_constant<client_message_type::game_action>, const sdlnet::ip_address &addr, const game_action &value) {
    auto it = find_lobby(addr);
    if (it == m_lobbies.end()) {
        throw std::runtime_error("Giocatore non in una lobby");
    }

    if (it->state != lobby_state::playing) {
        throw std::runtime_error("Lobby non in gioco");
    }

    auto user_it = it->users.find(addr);
    if (user_it == it->users.end()) {
        throw std::runtime_error("Giocatore non in questa lobby");
    }

    enums::visit([&]<game_action_type T>(enums::enum_constant<T> tag, auto && ... args) {
        it->game.handle_action(tag, user_it->second.controlling, std::forward<decltype(args)>(args) ...);
    }, value);
    it->send_updates(*this);
}

void lobby::send_updates(game_manager &mgr) {
    while (!game.m_updates.empty()) {
        const auto &data = game.m_updates.front();
        const auto &addr = std::ranges::find(users, data.first, [](const auto &pair) { return pair.second.controlling; })->first;
        mgr.send_message<server_message_type::game_update>(addr, data.second);
        game.m_updates.pop_front();
    }
}

void lobby::start_game() {
    game_options opts;
    opts.nplayers = users.size();
    
    game.start_game(opts);

    auto it = users.begin();
    for (auto &p : game.m_players) {
        it->second.controlling = &p;
        ++it;
    }
}