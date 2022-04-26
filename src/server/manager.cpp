#include "manager.h"

#include <iostream>
#include <stdexcept>

#include "net_enums.h"
#include "utils/json_serial.h"

using namespace banggame;
using namespace enums::flag_operators;

struct lobby_error : std::runtime_error {
    using std::runtime_error::runtime_error;
};

game_manager::game_manager(const std::filesystem::path &base_path)
    : all_cards(base_path) {}

void game_manager::handle_message(int client_id, const client_message &msg) {
    try {
        enums::visit_indexed([&](enums::enum_tag_for<client_message_type> auto tag, auto && ... args) {
            if constexpr (requires { handle_message(tag, client_id, std::forward<decltype(args)>(args) ...); }) {
                handle_message(tag, client_id, std::forward<decltype(args)>(args) ...);
            } else if (auto it = users.find(client_id); it != users.end()) {
                handle_message(tag, it, std::forward<decltype(args)>(args) ...);
            } else {
                throw invalid_message{};
            }
        }, msg);
    } catch (const lobby_error &e) {
        send_message<server_message_type::lobby_error>(client_id, e.what());
    } catch (const game_error &e) {
        send_message<server_message_type::game_update>(client_id, enums::enum_tag_t<game_update_type::game_error>(), e);
    }
}

void game_manager::tick() {
    for (auto it = m_lobbies.begin(); it != m_lobbies.end(); ++it) {
        auto &l = it->second;
        if (l.state == lobby_state::playing) {
            l.game.tick();
        }
        l.send_updates(*this);
        if (l.state == lobby_state::finished) {
            send_lobby_update(it);
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
    if (users.try_emplace(client_id, args.user_name, args.profile_image).second) {
        send_message<server_message_type::client_accepted>(client_id, client_id);
    }
}

lobby_data game_manager::make_lobby_data(lobby_ptr it) {
    const lobby &l = it->second;
    lobby_data obj;
    obj.lobby_id = it->first;
    obj.name = l.name;
    obj.num_players = l.users.size();
    obj.state = l.state;
    return obj;
}

void game_manager::send_lobby_update(lobby_ptr it) {
    auto msg = make_message<server_message_type::lobby_update>(make_lobby_data(it));
    for (int client_id : users | std::views::keys) {
        m_out_queue.emplace_back(client_id, msg);
    }
}

void game_manager::HANDLE_MESSAGE(lobby_list, user_ptr user) {
    for (auto it = m_lobbies.begin(); it != m_lobbies.end(); ++it) {
        send_message<server_message_type::lobby_update>(user->first, make_lobby_data(it));
    }
}

void game_manager::HANDLE_MESSAGE(lobby_make, user_ptr user, const lobby_info &value) {
    if (user->second.in_lobby != lobby_ptr{}) {
        throw lobby_error("ERROR_PLAYER_IN_LOBBY");
    }

    auto lobby_it = m_lobbies.try_emplace(++m_lobby_counter).first;
    auto &new_lobby = lobby_it->second;

    new_lobby.users.push_back(user);
    user->second.in_lobby = lobby_it;

    static_cast<lobby_info &>(new_lobby) = value;
    new_lobby.owner = user;
    new_lobby.state = lobby_state::waiting;
    send_lobby_update(lobby_it);

    send_message<server_message_type::lobby_entered>(user->first, value, user->first);
    send_message<server_message_type::lobby_add_user>(user->first, user->first, user->second.name, user->second.profile_image);
}

void game_manager::HANDLE_MESSAGE(lobby_edit, user_ptr user, const lobby_info &args) {
    if (user->second.in_lobby == lobby_ptr{}) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    auto &lobby = user->second.in_lobby->second;

    if (lobby.owner != user) {
        throw lobby_error("ERROR_PLAYER_NOT_LOBBY_OWNER");
    }

    if (lobby.state != lobby_state::waiting) {
        throw lobby_error("ERROR_LOBBY_NOT_WAITING");
    }

    static_cast<lobby_info &>(lobby) = args;
    for (user_ptr p : lobby.users) {
        if (p != user) {
            send_message<server_message_type::lobby_edited>(p->first, args);
        }
    }
}

void game_manager::HANDLE_MESSAGE(lobby_join, user_ptr user, const lobby_join_args &value) {
    auto lobby_it = m_lobbies.find(value.lobby_id);
    if (lobby_it == m_lobbies.end()) {
        throw lobby_error("ERROR_INVALID_LOBBY");
    }

    auto &lobby = lobby_it->second;
    if (lobby.users.size() < lobby_max_players) {
        lobby.users.emplace_back(user);
        user->second.in_lobby = lobby_it;
        send_lobby_update(lobby_it);

        send_message<server_message_type::lobby_entered>(user->first, lobby, lobby.owner->first);
        for (user_ptr p : lobby.users) {
            if (p != user) {
                send_message<server_message_type::lobby_add_user>(p->first, user->first, user->second.name, user->second.profile_image);
            }
            send_message<server_message_type::lobby_add_user>(user->first, p->first, p->second.name, p->second.profile_image);
        }
        if (lobby.state != lobby_state::waiting) {
            send_message<server_message_type::game_started>(user->first, lobby.game.m_options);

            player *controlling = lobby.game.find_disconnected_player();
            if (controlling) {
                controlling->client_id = user->first;
                broadcast_message<server_message_type::game_update>(lobby,
                    enums::enum_tag<game_update_type::player_add>, controlling->id, controlling->client_id);
            }

            for (const player &p : lobby.game.m_players) {
                if (&p != controlling && (p.alive() || lobby.game.has_expansion(card_expansion_type::ghostcards))) {
                    send_message<server_message_type::game_update>(user->first, enums::enum_tag<game_update_type::player_add>,
                        p.id, p.client_id);
                }
            }

            for (const auto &msg : lobby.game.get_game_state_updates(controlling)) {
                send_message<server_message_type::game_update>(user->first, msg);
            }
        }
    }
}

void game_manager::client_disconnected(int client_id) {
    if (auto it = users.find(client_id); it != users.end()) {
        handle_message(MESSAGE_TAG(lobby_leave){}, it);
        users.erase(it);
    }
}

bool game_manager::client_validated(int client_id) const {
    return users.find(client_id) != users.end();
}

void game_manager::HANDLE_MESSAGE(lobby_leave, user_ptr user) {
    if (user->second.in_lobby == lobby_ptr{}) return;

    auto lobby_it = std::exchange(user->second.in_lobby, lobby_ptr{});
    auto &lobby = lobby_it->second;

    if (auto it = std::ranges::find(lobby.game.m_players, user->first, &player::client_id); it != lobby.game.m_players.end()) {
        it->client_id = 0;
    }
    
    broadcast_message<server_message_type::lobby_remove_user>(lobby, user->first);
    lobby.users.erase(std::ranges::find(lobby.users, user));
    
    if (lobby.state == lobby_state::waiting && user == lobby.owner) {
        for (user_ptr u : lobby.users) {
            broadcast_message<server_message_type::lobby_remove_user>(lobby, u->first);
            u->second.in_lobby = lobby_ptr{};
        }
        lobby.users.clear();
    }

    send_lobby_update(lobby_it);

    if (lobby.users.empty()) {
        m_lobbies.erase(lobby_it);
    }
}

void game_manager::HANDLE_MESSAGE(lobby_chat, user_ptr user, const lobby_chat_client_args &value) {
    if (user->second.in_lobby == lobby_ptr{}) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    broadcast_message<server_message_type::lobby_chat>(user->second.in_lobby->second, user->first, value.message);
}

void game_manager::HANDLE_MESSAGE(lobby_return, user_ptr user) {
    if (user->second.in_lobby == lobby_ptr{}) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    auto &lobby = user->second.in_lobby->second;

    if (user != lobby.owner) {
        throw lobby_error("ERROR_PLAYER_NOT_LOBBY_OWNER");
    }

    if (lobby.state != lobby_state::finished) {
        throw lobby_error("ERROR_LOBBY_NOT_FINISHED");
    }

    lobby.state = lobby_state::waiting;
    send_lobby_update(user->second.in_lobby);

    broadcast_message<server_message_type::lobby_entered>(lobby, lobby, user->first);
    for (user_ptr p : lobby.users) {
        broadcast_message<server_message_type::lobby_add_user>(lobby, p->first, p->second.name, p->second.profile_image);
    }
}

void game_manager::HANDLE_MESSAGE(game_start, user_ptr user) {
    if (user->second.in_lobby == lobby_ptr{}) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }

    auto &lobby = user->second.in_lobby->second;

    if (user != lobby.owner) {
        throw lobby_error("ERROR_PLAYER_NOT_LOBBY_OWNER");
    }

    if (lobby.state != lobby_state::waiting) {
        throw lobby_error("ERROR_LOBBY_NOT_WAITING");
    }

    if (lobby.users.size() <= 1) {
        throw lobby_error("ERROR_NOT_ENOUGH_PLAYERS");
    }

    lobby.state = lobby_state::playing;
    send_lobby_update(user->second.in_lobby);

    lobby.start_game(*this, all_cards);
}

void game_manager::HANDLE_MESSAGE(game_action, user_ptr user, const game_action &value) {
#ifdef DEBUG_PRINT_GAME_UPDATES
    std::cout << "/*** GAME ACTION *** ID = " << user->first << " ***/ " << json::serialize(value) << '\n';
#endif
    if (user->second.in_lobby == lobby_ptr{}) {
        throw lobby_error("ERROR_PLAYER_NOT_IN_LOBBY");
    }
    auto &lobby = user->second.in_lobby->second;

    if (lobby.state != lobby_state::playing) {
        throw lobby_error("ERROR_LOBBY_NOT_PLAYING");
    }

    if (auto it = std::ranges::find(lobby.game.m_players, user->first, &player::client_id); it != lobby.game.m_players.end()) {
        enums::visit_indexed([&](auto tag, auto && ... args) {
            lobby.game.handle_action(tag, &*it, std::forward<decltype(args)>(args) ...);
        }, value);
    } else {
        throw lobby_error("ERROR_USER_NOT_CONTROLLING_PLAYER");
    }
}

void lobby::send_updates(game_manager &mgr) {
    while (state == lobby_state::playing && !game.m_updates.empty()) {
        auto &[target, update] = game.m_updates.front();
        if (update.is(game_update_type::game_over)) {
            state = lobby_state::finished;
        }
        switch (target.type) {
        case update_target::private_update:
            mgr.send_message<server_message_type::game_update>(target.target->client_id, std::move(update));
            break;
        case update_target::inv_private_update:
            for (auto it : users) {
                if (it->first != target.target->client_id) {
                    mgr.send_message<server_message_type::game_update>(it->first, update);
                }
            }
            break;
        case update_target::public_update:
            mgr.broadcast_message<server_message_type::game_update>(*this, std::move(update));
            break;
        case update_target::spectator_update:
            for (auto it : users) {
                if (std::ranges::none_of(game.m_players, [client_id = it->first](const player &p) {
                    return p.client_id == client_id;
                })) {
                    mgr.send_message<server_message_type::game_update>(it->first, update);
                }
            }
            break;
        }
        game.m_updates.pop_front();
    }
}

void lobby::start_game(game_manager &mgr, const banggame::all_cards_t &all_cards) {
    game_options opts;
    opts.expansions = expansions;
    opts.keep_last_card_shuffling = false;

    mgr.broadcast_message<server_message_type::game_started>(*this, opts);

    game = {};
    
    std::vector<player *> ids;
    for (const auto &_ : users) {
        ids.push_back(&game.m_players.emplace(&game, game.m_players.first_available_id()));
    }
    std::ranges::shuffle(ids, game.rng);

    auto it = users.begin();
    for (player *p : ids) {
        p->client_id = (*it)->first;
        game.add_public_update<game_update_type::player_add>(p->id, p->client_id);
        ++it;
    }

    game.start_game(opts, all_cards);
}