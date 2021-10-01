#ifndef __CLIENT_MANAGER_H__
#define __CLIENT_MANAGER_H__

#include <list>

#include "common/net_enums.h"

#include "scenes/connect.h"
#include "scenes/lobby_list.h"
#include "scenes/make_lobby.h"
#include "scenes/lobby.h"
#include "scenes/game/game.h"

DEFINE_ENUM_TYPES(scene_type,
    (connect, connect_scene)
    (lobby_list, lobby_list_scene)
    (make_lobby, make_lobby_scene)
    (lobby, lobby_scene)
    (game, banggame::game_scene)
)

class game_manager {
public:
    game_manager();
    ~game_manager();

    void resize(int width, int height);

    void render(sdl::renderer &renderer);

    void handle_event(const sdl::event &event);

    template<client_message_type E, typename ... Ts>
    void add_message(Ts && ... args) {
        Json::Value message = Json::objectValue;
        message["type"] = json::serialize(E);
        if constexpr (enums::has_type<E>) {
            message["value"] = json::serialize(enums::enum_type_t<E>{ std::forward<Ts>(args) ... });
        }
        m_out_queue.emplace_back(std::move(message));
    }

    void update_net();

    void connect(const std::string &host);
    void disconnect();

    template<scene_type E>
    auto *switch_scene() {
        if (m_scene) {
            delete m_scene;
        }
        auto *scene = new enums::enum_type_t<E>(this);
        scene->resize(m_width, m_height);
        m_scene = scene;
        return scene;
    }

private:
    void handle_message(enums::enum_constant<server_message_type::game_error>, const game_error &args);
    void handle_message(enums::enum_constant<server_message_type::lobby_list>, const std::vector<lobby_data> &args);
    void handle_message(enums::enum_constant<server_message_type::lobby_entered>, const lobby_entered_args &args);
    void handle_message(enums::enum_constant<server_message_type::lobby_players>, const std::vector<lobby_player_data> &args);
    void handle_message(enums::enum_constant<server_message_type::lobby_joined>, const lobby_player_data &args);
    void handle_message(enums::enum_constant<server_message_type::lobby_left>, const lobby_left_args &args);
    void handle_message(enums::enum_constant<server_message_type::lobby_chat>, const lobby_chat_args &args);
    void handle_message(enums::enum_constant<server_message_type::game_started>);
    void handle_message(enums::enum_constant<server_message_type::game_update>, const banggame::game_update &args);

    scene_base *m_scene = nullptr;

    std::list<Json::Value> m_out_queue;

    sdlnet::tcp_socket sock;
    sdlnet::socket_set sock_set{1};

    int m_width;
    int m_height;

    int m_user_own_id = 0;
};

#endif