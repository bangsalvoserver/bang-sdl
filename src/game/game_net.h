#ifndef __GAME_NET_H__
#define __GAME_NET_H__

#include <deque>

#include "player.h"
#include "format_str.h"
#include "server/net_enums.h"

namespace banggame {

    inline std::vector<card_backface> make_id_vector(auto &&range) {
        auto view = range | std::views::transform([](const card *c) {
            return card_backface{c->id, c->deck};
        });
        return {view.begin(), view.end()};
    };

    struct update_target {
        player *target;
        enum update_target_type {
            private_update,
            inv_private_update,
            public_update,
            spectator_update
        } type;

        update_target(player *target, update_target_type type = private_update) : target(target), type(type) {}
        update_target(update_target_type type) : target(nullptr), type(type) {}
    };

    struct game_net_manager {
        std::deque<std::pair<update_target, game_update>> m_updates;

        template<game_update_type E, typename ... Ts>
        void add_private_update(update_target p, Ts && ... args) {
            m_updates.emplace_back(std::piecewise_construct,
                std::make_tuple(p),
                std::make_tuple(enums::enum_tag<E>, std::forward<Ts>(args) ... ));
        }

        template<game_update_type E, typename ... Ts>
        void add_public_update(Ts && ... args) {
            add_private_update<E>(update_target::public_update, std::forward<Ts>(args) ... );
        }

        template<typename ... Ts>
        void add_private_log(update_target p, Ts && ... args) {
            add_private_update<game_update_type::game_log>(p, std::forward<Ts>(args) ... );
        }

        template<typename ... Ts>
        void add_log(Ts && ... args) {
            add_private_log(update_target::public_update, std::forward<Ts>(args) ... );
        }
    };

}

#endif