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

    struct game_net_manager {
        std::deque<std::pair<player *, game_update>> m_updates;
        std::deque<game_formatted_string> m_saved_log;

        template<game_update_type E, typename ... Ts>
        void add_private_update(player *p, Ts && ... args) {
            m_updates.emplace_back(std::piecewise_construct,
                std::make_tuple(p),
                std::make_tuple(enums::enum_tag<E>, std::forward<Ts>(args) ...));
        }

        template<game_update_type E, typename ... Ts>
        void add_public_update(const Ts & ... args) {
            add_private_update<E>(nullptr, args ...);
        }

        template<typename ... Ts>
        void add_log(Ts && ... args) {
            add_public_update<game_update_type::game_log>(m_saved_log.emplace_back(std::forward<Ts>(args) ...));
        }

        void handle_action(enums::enum_tag_t<game_action_type::pick_card>, player *p, const pick_card_args &args) {
            p->pick_card(args);
        }

        void handle_action(enums::enum_tag_t<game_action_type::play_card>, player *p, const play_card_args &args) {
            p->play_card(args);
        }

        void handle_action(enums::enum_tag_t<game_action_type::respond_card>, player *p, const play_card_args &args) {
            p->respond_card(args);
        }

        void handle_action(enums::enum_tag_t<game_action_type::prompt_respond>, player *p, bool response) {
            p->prompt_response(response);
        }
    };

}

#endif