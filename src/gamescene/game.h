#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "scenes/scene_base.h"
#include "sounds_pak.h"

#include "animation.h"
#include "game_ui.h"

#include "target_finder.h"

#include "utils/id_map.h"
#include "utils/utils.h"

#include <deque>
#include <random>

namespace banggame {

    class game_context_view {
    public:
        util::id_map<card_view> cards;
        util::id_map<player_view> players;

        card_view *find_card(int id) const {
            if (auto it = cards.find(id); it != cards.end()) {
                return &*it;
            }
            throw std::runtime_error(fmt::format("client.find_card: ID {} not found", id));
        }

        player_view *find_player(int id) const {
            if (auto it = players.find(id); it != players.end()) {
                return &*it;
            }
            throw std::runtime_error(fmt::format("client.find_player: ID {} not found", id));
        }
    };

    class game_scene : public scene_base,
    public message_handler<server_message_type::game_update>,
    public message_handler<server_message_type::lobby_owner>,
    public message_handler<server_message_type::lobby_error> {
    public:
        game_scene(client_manager *parent);
        
        void refresh_layout() override;
        void tick(duration_type time_elapsed) override;
        void render(sdl::renderer &renderer) override;
        void handle_event(const sdl::event &event) override;

        void play_sound(std::string_view sound_id);

        void handle_message(SRV_TAG(game_update), const json::json &update) override;
        void handle_message(SRV_TAG(lobby_owner), const user_id_args &args) override;
        void handle_message(SRV_TAG(lobby_error), const std::string &message) override;

        const game_context_view &context() const {
            return m_context;
        }

        const target_finder &get_target_finder() const {
            return m_target;
        }

        bool has_game_flags(game_flags flags) const {
            return bool(m_game_flags & flags);
        }

    private:
        void handle_game_update(UPD_TAG(game_error),       const game_string &args);
        void handle_game_update(UPD_TAG(game_log),         const game_string &args);
        void handle_game_update(UPD_TAG(game_prompt),      const game_string &args);
        void handle_game_update(UPD_TAG(add_cards),        const add_cards_update &args);
        void handle_game_update(UPD_TAG(remove_cards),     const remove_cards_update &args);
        void handle_game_update(UPD_TAG(move_card),        const move_card_update &args);
        void handle_game_update(UPD_TAG(add_cubes),        const add_cubes_update &args);
        void handle_game_update(UPD_TAG(move_cubes),       const move_cubes_update &args);
        void handle_game_update(UPD_TAG(move_scenario_deck), const move_scenario_deck_update &args);
        void handle_game_update(UPD_TAG(move_train),       const move_train_update &args);
        void handle_game_update(UPD_TAG(deck_shuffled),    const deck_shuffled_update &args);
        void handle_game_update(UPD_TAG(show_card),        const show_card_update &args);
        void handle_game_update(UPD_TAG(hide_card),        const hide_card_update &args);
        void handle_game_update(UPD_TAG(tap_card),         const tap_card_update &args);
        void handle_game_update(UPD_TAG(flash_card),       const flash_card_update &args);
        void handle_game_update(UPD_TAG(short_pause),      const short_pause_update &args);
        void handle_game_update(UPD_TAG(player_add),       const player_add_update &args);
        void handle_game_update(UPD_TAG(player_order),     const player_order_update &args);
        void handle_game_update(UPD_TAG(player_user),      const player_user_update &args);
        void handle_game_update(UPD_TAG(player_hp),        const player_hp_update &args);
        void handle_game_update(UPD_TAG(player_gold),      const player_gold_update &args);
        void handle_game_update(UPD_TAG(player_show_role), const player_show_role_update &args);
        void handle_game_update(UPD_TAG(player_status),    const player_status_update &args);
        void handle_game_update(UPD_TAG(switch_turn),      player_view *player);
        void handle_game_update(UPD_TAG(request_status),   const request_status_args &args);
        void handle_game_update(UPD_TAG(status_ready),     const status_ready_args &args);
        void handle_game_update(UPD_TAG(game_flags),       const game_flags &args);
        void handle_game_update(UPD_TAG(play_sound),       const std::string &sound_id);
        void handle_game_update(UPD_TAG(status_clear));

        template<typename T>
        void add_animation(anim_duration_type duration, auto && ... args) {
            if constexpr (requires (T anim) { anim.end(); }) {
                if (duration <= anim_duration_type{0}) {
                    T{FWD(args) ... }.end();
                    return;
                }
            }
            m_animations.emplace_back(duration, std::in_place_type<T>, FWD(args) ... );
        }

        void move_player_views(anim_duration_type duration = {});

        std::tuple<pocket_type, player_view *, card_view *> find_card_at(sdl::point pt) const;

        pocket_view &get_pocket(pocket_type pocket, player_view *player = nullptr);

        sounds_pak m_sounds;
        card_textures m_card_textures;

        game_context_view m_context;

        game_ui m_ui;

        target_finder m_target;

        std::deque<json::json> m_pending_updates;
        std::deque<animation> m_animations;

        counting_pocket m_shop_deck{pocket_type::shop_deck};
        point_pocket_view m_shop_discard{pocket_type::shop_discard};
        point_pocket_view m_hidden_deck{pocket_type::hidden_deck};
        flipped_pocket m_shop_selection{options.shop_selection_width, pocket_type::shop_selection};
        card_choice_pocket m_card_choice;
        
        table_cube_pile m_cubes;

        counting_pocket m_main_deck{pocket_type::main_deck};
        point_pocket_view m_discard_pile{pocket_type::discard_pile};

        point_pocket_view m_scenario_card{pocket_type::scenario_card};
        point_pocket_view m_wws_scenario_card{pocket_type::wws_scenario_card};

        train_pocket m_stations{pocket_type::stations};
        train_pocket m_train{pocket_type::train};
        counting_pocket m_train_deck{pocket_type::train_deck};

        wide_pocket m_selection{options.selection_width, pocket_type::selection};

        button_row_pocket m_button_row{this};

        int m_train_position = 0;

        std::vector<player_view *> m_alive_players;
        std::vector<player_view *> m_dead_players;

        sdl::point m_mouse_pt;
        duration_type m_mouse_motion_timer{0};
        bool m_middle_click = false;
        card_view *m_overlay = nullptr;
        
        player_view *m_player_self = nullptr;
        player_view *m_playing = nullptr;
        player_view *m_scenario_player = nullptr;
        player_view *m_wws_scenario_player = nullptr;

        std::optional<game_style_tracker> m_turn_border;

        game_flags m_game_flags{};

        cube_pile_base &get_cube_pile(card_view *card);

        friend class game_ui;
        friend class target_finder;
        friend class player_view;
        friend class button_row_pocket;
    };

    std::string evaluate_format_string(const game_string &str);

}

#endif