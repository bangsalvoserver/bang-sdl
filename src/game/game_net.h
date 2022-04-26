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

    class update_target {
    private:
        std::vector<player *> m_targets;
        bool m_inclusive;

        update_target(bool inclusive, std::same_as<player *> auto ... targets)
            : m_targets{targets ...}, m_inclusive{inclusive} {}

    public:
        static update_target includes(std::same_as<player *> auto ... targets) {
            return update_target(true, targets...);
        }

        static update_target excludes(std::same_as<player *> auto ... targets) {
            return update_target(false, targets...);
        }

        void add(player *target) {
            m_targets.push_back(target);
        }

        bool matches(int client_id) const {
            return (std::ranges::find(m_targets, client_id, &player::client_id) != m_targets.end()) == m_inclusive;
        }
    };

    struct game_net_manager {
        std::deque<std::pair<update_target, game_update>> m_updates;
        std::deque<std::pair<update_target, game_formatted_string>> m_saved_log;

        template<game_update_type E, typename ... Ts>
        void add_update(update_target target, Ts && ... args) {
            m_updates.emplace_back(std::piecewise_construct,
                std::make_tuple(std::move(target)),
                std::make_tuple(enums::enum_tag<E>, std::forward<Ts>(args) ... ));
        }

        template<game_update_type E, typename ... Ts>
        void add_update(Ts && ... args) {
            add_update<E>(update_target::excludes(), std::forward<Ts>(args) ... );
        }

        template<typename ... Ts>
        void add_log(update_target target, Ts && ... args) {
            const auto &log = m_saved_log.emplace_back(std::piecewise_construct,
                std::make_tuple(target), std::make_tuple(std::forward<Ts>(args) ... ));
            add_update<game_update_type::game_log>(std::move(target), log.second);
        }

        template<typename ... Ts>
        void add_log(Ts && ... args) {
            add_log(update_target::excludes(), std::forward<Ts>(args) ... );
        }
    };

}

#endif