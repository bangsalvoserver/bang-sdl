#include "manager.h"

#include <iostream>
#include <stdexcept>

#include "net_enums.h"

using namespace banggame;
using namespace enums::flag_operators;
using namespace std::string_literals;

struct lobby_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

game_manager::game_manager(const std::filesystem::path &base_path)
    : all_cards(make_all_cards(base_path)) {}

void game_manager::handle_message(int client_id, const client_message &msg) {
    try {
        enums::visit_indexed([&](auto enum_const, auto && ... args) {
            if constexpr (requires { handle_message(enum_const, client_id, std::forward<decltype(args)>(args) ...); }) {
                handle_message(enum_const, client_id, std::forward<decltype(args)>(args) ...);
            } else if (auto it = users.find(client_id); it != users.end()) {
                handle_message(enum_const, &it->second, std::forward<decltype(args)>(args) ...);
            } else {
                print_error("Invalid connection"s);
            }
        }, msg);
    } catch (const lobby_error &e) {
        send_message<server_message_type::lobby_error>(client_id, e.what());
    } catch (const game_error &e) {
        send_message<server_message_type::game_update>(client_id, enums::enum_constant<game_update_type::game_error>(), e);
    } catch (const std::exception &e) {
        print_error("Error: "s + e.what());
    }
}

void game_manager::tick() {
    for (auto &l : m_lobbies) {
        if (l.state == lobby_state::playing) {
            l.game.tick();
        }
        l.send_updates(*this);
        if (l.state == lobby_state::finished) {
            send_lobby_update(l);
        }
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


void game_manager::HANDLE_MESSAGE(connect, int client_id, const connect_args &args) {
    if (users.try_emplace(client_id, client_id, args.user_name, args.profile_image).second) {
        send_message<server_message_type::client_accepted>(client_id);
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
    for (int client_id : users | std::views::keys) {
        m_out_queue.emplace_back(client_id, msg);
    }
}

void game_manager::HANDLE_MESSAGE(lobby_list, game_user *user) {
    std::vector<lobby_data> vec;
    for (const auto &lobby : m_lobbies) {
        vec.push_back(make_lobby_data(lobby));
    }
    send_message<server_message_type::lobby_list>(user->client_id, std::move(vec));
}

void game_manager::HANDLE_MESSAGE(lobby_make, game_user *user, const lobby_info &value) {
    if (user->in_lobby) {
        throw lobby_error("ERROR_PLAYER_IN_LOBBY");
    }

    lobby &new_lobby = m_lobbies.emplace_back();
    new_lobby.users.push_back(user);
    user->in_lobby = &new_lobby;

    new_lobby.owner = user;
    new_lobby.name = value.name;
    new_lobby.state = lobby_state::waiting;
    new_lobby.expansions = value.expansions;
    send_lobby_update(new_lobby);

    send_message<server_message_type::lobby_entered>(user->client_id, value, user->client_id, user->client_id);
}

void game_manager::HANDLE_MESSAGE(lobby_edit, game_user *user, const lobby_info &args) {
    lobby *lobby_ptr = user->in_lobby;
    if (!lobby_ptr) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    if (lobby_ptr->owner != user) {
        throw lobby_error("ERROR_PLAYER_NOT_LOBBY_OWNER");
    }

    if (lobby_ptr->state != lobby_state::waiting) {
        throw lobby_error("ERROR_LOBBY_NOT_WAITING");
    }

    lobby_ptr->name = args.name;
    lobby_ptr->expansions = args.expansions;
    for (game_user *p : lobby_ptr->users) {
        if (p != user) {
            send_message<server_message_type::lobby_edited>(p->client_id, args);
        }
    }
}

void game_manager::HANDLE_MESSAGE(lobby_join, game_user *user, const lobby_join_args &value) {
    auto lobby_it = std::ranges::find(m_lobbies, value.lobby_id, &lobby::id);
    if (lobby_it == m_lobbies.end()) {
        throw lobby_error("ERROR_INVALID_LOBBY");
    }

    if (lobby_it->users.size() < lobby_max_players) {
        lobby_it->users.emplace_back(user);
        user->in_lobby = &*lobby_it;
        send_lobby_update(*lobby_it);

        for (game_user *p : lobby_it->users) {
            if (p == user) {
                send_message<server_message_type::lobby_entered>(p->client_id, lobby_info{lobby_it->name, lobby_it->expansions}, user->client_id, lobby_it->owner->client_id);
            } else {
                send_message<server_message_type::lobby_joined>(p->client_id, user->client_id, user->name, user->profile_image);
            }
        }
        if (lobby_it->state != lobby_state::waiting) {
            auto dc_player = std::ranges::find_if(lobby_it->game.m_players, [&](const player &p) {
                return std::ranges::find(lobby_it->users, &p, &game_user::controlling) == lobby_it->users.end();
            });
            if (dc_player != lobby_it->game.m_players.end()) {
                user->controlling = &*dc_player;
            }

            std::vector<lobby_player_data> vec;
            for (game_user *l_u : lobby_it->users) {
                vec.emplace_back(l_u->client_id, l_u->name, l_u->profile_image);
            }
            send_message<server_message_type::lobby_players>(user->client_id, std::move(vec));

            send_message<server_message_type::game_started>(user->client_id, lobby_it->game.m_options.expansions);
            for (const player &p : lobby_it->game.m_players) {
                auto u_it = std::ranges::find(lobby_it->users, &p, &game_user::controlling);
                send_message<server_message_type::game_update>(user->client_id, enums::enum_constant<game_update_type::player_add>{},
                    p.id, u_it == lobby_it->users.end() ? 0 : (*u_it)->client_id);
            }
            for (const auto &msg : lobby_it->game.get_game_state_updates(user->controlling)) {
                send_message<server_message_type::game_update>(user->client_id, msg);
            }

            if (user->controlling) {
                broadcast_message<server_message_type::game_update>(*lobby_it,
                    enums::enum_constant<game_update_type::player_add>{}, user->controlling->id, user->client_id);
            }
        }
    }
}

void game_manager::HANDLE_MESSAGE(lobby_players, game_user *user) {
    if (!user->in_lobby) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    std::vector<lobby_player_data> vec;
    for (game_user *u : user->in_lobby->users) {
        vec.emplace_back(u->client_id, u->name, u->profile_image);
    }

    send_message<server_message_type::lobby_players>(user->client_id, std::move(vec));
}

void game_manager::client_disconnected(int client_id) {
    if (auto it = users.find(client_id); it != users.end()) {
        handle_message(MESSAGE_TAG(lobby_leave){}, &it->second);
        users.erase(it);
    }
}

void game_manager::HANDLE_MESSAGE(lobby_leave, game_user *user) {
    if (!user->in_lobby) return;
    
    auto lobby_it = std::ranges::find(m_lobbies, user->in_lobby, [](const lobby &l) { return &l; });
    if (lobby_it != m_lobbies.end()) {
        auto player_it = std::ranges::find(lobby_it->users, user);
        broadcast_message<server_message_type::lobby_left>(*lobby_it, (*player_it)->client_id);
        lobby_it->users.erase(player_it);
        user->in_lobby = nullptr;
        send_lobby_update(*lobby_it);
        
        if (lobby_it->users.empty()) {
            m_lobbies.erase(lobby_it);
        } else if (lobby_it->state == lobby_state::waiting && user == lobby_it->owner) {
            for (game_user *u : lobby_it->users) {
                broadcast_message<server_message_type::lobby_left>(*lobby_it, u->client_id);
                u->in_lobby = nullptr;
            }
            m_lobbies.erase(lobby_it);
        }
    }
}

void game_manager::HANDLE_MESSAGE(lobby_chat, game_user *user, const lobby_chat_client_args &value) {
    if (!user->in_lobby) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    broadcast_message<server_message_type::lobby_chat>(*user->in_lobby, user->client_id, value.message);
}

void game_manager::HANDLE_MESSAGE(game_start, game_user *user) {
    lobby *lobby_ptr = user->in_lobby;
    if (!lobby_ptr) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    if (user != lobby_ptr->owner) {
        throw lobby_error("ERROR_PLAYER_NOT_LOBBY_OWNER");
    }

    if (lobby_ptr->state == lobby_state::playing) {
        throw lobby_error("ERROR_LOBBY_NOT_WAITING");
    }

    if (lobby_ptr->users.size() <= 1) {
        throw lobby_error("ERROR_NOT_ENOUGH_PLAYERS");
    }

    lobby_ptr->state = lobby_state::playing;
    send_lobby_update(*lobby_ptr);

    broadcast_message<server_message_type::game_started>(*lobby_ptr, lobby_ptr->expansions | card_expansion_type::base);

    lobby_ptr->start_game(all_cards);
}

void game_manager::HANDLE_MESSAGE(game_action, game_user *user, const game_action &value) {
    lobby *lobby_ptr = user->in_lobby;
    if (!lobby_ptr) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    if (lobby_ptr->state != lobby_state::playing) {
        throw lobby_error("ERROR_LOBBY_NOT_PLAYING");
    }

    if (!user->controlling) {
        throw lobby_error("ERROR_USER_NOT_CONTROLLING_PLAYER");
    }

    enums::visit_indexed([&](auto tag, auto && ... args) {
        lobby_ptr->game.handle_action(tag, user->controlling, std::forward<decltype(args)>(args) ...);
    }, value);
}

void lobby::send_updates(game_manager &mgr) {
    while (state == lobby_state::playing && !game.m_updates.empty()) {
        const auto &data = game.m_updates.front();
        if (data.first) {
            if (auto it = std::ranges::find(users, data.first, &game_user::controlling); it != users.end()) {
                mgr.send_message<server_message_type::game_update>((*it)->client_id, data.second);
            }
        } else {
            mgr.broadcast_message<server_message_type::game_update>(*this, data.second);
        }
        if (data.second.is(game_update_type::game_over)) {
            state = lobby_state::finished;
        } else {
            game.m_updates.pop_front();
        }
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
        (*it)->controlling = p;
        game.add_public_update<game_update_type::player_add>(p->id, (*it)->client_id);
        ++it;
    }

    game.start_game(opts, all_cards);
}