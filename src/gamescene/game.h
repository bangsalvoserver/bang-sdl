#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "../scenes/scene_base.h"

#include "animation.h"
#include "game_ui.h"

#include "target_finder.h"

#include "utils/id_map.h"

#include <deque>
#include <random>

namespace banggame {

    class game_scene : public scene_base {
    public:
        game_scene(client_manager *parent);
        
        void refresh_layout() override;
        void tick(duration_type time_elapsed) override;
        void render(sdl::renderer &renderer) override;
        void handle_event(const sdl::event &event) override;

        void handle_game_update(const game_update &update);

    private:
        void handle_game_update(UPD_TAG(game_over),        const game_over_update &args);
        void handle_game_update(UPD_TAG(game_error),       const game_string &args);
        void handle_game_update(UPD_TAG(game_log),         const game_string &args);
        void handle_game_update(UPD_TAG(game_prompt),      const game_string &args);
        void handle_game_update(UPD_TAG(add_cards),        const add_cards_update &args);
        void handle_game_update(UPD_TAG(remove_cards),     const remove_cards_update &args);
        void handle_game_update(UPD_TAG(move_card),        const move_card_update &args);
        void handle_game_update(UPD_TAG(add_cubes),        const add_cubes_update &args);
        void handle_game_update(UPD_TAG(move_cubes),       const move_cubes_update &args);
        void handle_game_update(UPD_TAG(move_scenario_deck), const move_scenario_deck_args &args);
        void handle_game_update(UPD_TAG(deck_shuffled),    const pocket_type &pocket);
        void handle_game_update(UPD_TAG(show_card),        const show_card_update &args);
        void handle_game_update(UPD_TAG(hide_card),        const hide_card_update &args);
        void handle_game_update(UPD_TAG(tap_card),         const tap_card_update &args);
        void handle_game_update(UPD_TAG(flash_card),       const flash_card_update &args);
        void handle_game_update(UPD_TAG(last_played_card), const card_id_args &args);
        void handle_game_update(UPD_TAG(player_add),       const player_add_update &args);
        void handle_game_update(UPD_TAG(player_user),      const player_user_update &args);
        void handle_game_update(UPD_TAG(player_remove),    const player_remove_update &args);
        void handle_game_update(UPD_TAG(player_hp),        const player_hp_update &args);
        void handle_game_update(UPD_TAG(player_gold),      const player_gold_update &args);
        void handle_game_update(UPD_TAG(player_show_role), const player_show_role_update &args);
        void handle_game_update(UPD_TAG(player_status),    const player_status_update &args);
        void handle_game_update(UPD_TAG(switch_turn),      const switch_turn_update &args);
        void handle_game_update(UPD_TAG(request_status),   const request_status_args &args);
        void handle_game_update(UPD_TAG(game_flags),       const game_flags &args);
        void handle_game_update(UPD_TAG(game_options),     const game_options &args);
        void handle_game_update(UPD_TAG(status_clear));
        void handle_game_update(UPD_TAG(confirm_play));

        template<typename T>
        void add_animation(int duration, auto && ... args) {
            m_animations.emplace_back(std::chrono::milliseconds{duration}, std::in_place_type<T>, FWD(args) ... );
        }

        void move_player_views(bool instant = true);

        void handle_card_click();
        void find_overlay();

        pocket_view &get_pocket(pocket_type pocket, int player_id = 0);

        card_textures m_card_textures;

        game_ui m_ui;
        target_finder m_target;

        std::deque<game_update> m_pending_updates;
        std::deque<animation> m_animations;

        counting_pocket m_shop_deck;
        point_pocket_view m_shop_discard;
        point_pocket_view m_hidden_deck;
        flipped_pocket m_shop_selection{options.shop_selection_width};
        wide_pocket m_shop_choice{options.shop_choice_width};
        
        table_cube_pile m_cubes;

        counting_pocket m_main_deck;
        point_pocket_view m_discard_pile;

        point_pocket_view m_scenario_card;

        wide_pocket m_selection{options.selection_width};

        pocket_view m_specials;

        util::id_map<card_view> m_cards;
        util::id_map<player_view> m_players;

        std::vector<player_view *> m_alive_players;
        std::vector<player_view *> m_dead_players;

        sdl::point m_mouse_pt;
        duration_type m_mouse_motion_timer{0};
        bool m_middle_click = false;
        card_view *m_overlay = nullptr;

        game_options m_game_options;
        
        player_view *m_player_self = nullptr;
        player_view *m_playing = nullptr;
        player_view *m_request_origin = nullptr;
        player_view *m_request_target = nullptr;
        player_view *m_scenario_player = nullptr;

        raii_editor<sdl::color> m_turn_border;

        player_role m_winner_role = player_role::unknown;
        game_flags m_game_flags{};

        cube_pile_base &find_cube_pile(int card_id);

        card_view *find_card(int id) {
            if (auto it = m_cards.find(id); it != m_cards.end()) {
                return &*it;
            }
            return nullptr;
        }

        player_view *find_player(int id) {
            if (auto it = m_players.find(id); it != m_players.end()) {
                return &*it;
            }
            return nullptr;
        }

        std::string evaluate_format_string(const game_string &str);

        friend class game_ui;
        friend class target_finder;
        friend class player_view;
    };

}

#endif