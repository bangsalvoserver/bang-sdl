#ifndef __TARGET_FINDER_H__
#define __TARGET_FINDER_H__

#include "player.h"

#include "game/holders.h"
#include "utils/nullable.h"
#include "utils/raii_editor.h"

#include <vector>

namespace banggame {

    class game_scene;

    using player_card_pair = std::pair<player_view *, card_view *>;
    using card_cube_pair = std::pair<card_view *, cube_widget *>;

    DEFINE_ENUM_VARIANT(target_variant, target_type,
        (player,                player_view *)
        (conditional_player,    nullable<player_view>)
        (card,                  card_view*)
        (extra_card,            nullable<card_view>)
        (cards_other_players,   std::vector<player_card_pair>)
        (cube,                  std::vector<card_cube_pair>)
    )
    
    using target_vector = std::vector<target_variant>;

    struct target_status {
        card_view *m_playing_card = nullptr;
        std::vector<card_view *> m_modifiers;

        target_vector m_targets;
        raii_editor_stack<sdl::color> m_target_borders;

        bool m_equipping = false;
        bool m_response = false;
    };

    class target_finder : private target_status {
    public:
        target_finder(game_scene *parent) : m_game(parent) {}

        bool can_respond_with(card_view *card) const;
        bool can_play_in_turn(pocket_type pocket, player_view *player, card_view *card) const;
        bool can_confirm() const;

        void set_picking_border(pocket_type pocket, player_view *player, card_view *card, sdl::color color);
        void set_response_highlights(const request_status_args &args);
        void clear_status();
        void clear_targets();

        void set_last_played_card(card_view *card) {
            m_last_played_card = card;
        }

        void set_forced_card(card_view *card);

        void confirm_play();
        bool waiting_confirm() const {
            return m_waiting_confirm;
        }

        bool is_card_clickable() const;
        bool send_pick_card(pocket_type pocket, player_view *player = nullptr, card_view *card = nullptr);

        void on_click_card(pocket_type pocket, player_view *player, card_view *card);
        bool on_click_player(player_view *player);

        void on_click_confirm();

        bool is_playing_card(card_view *card) const {
            return m_playing_card == card;
        }
        
        void send_prompt_response(bool response);
    
    private:
        bool is_bangcard(card_view *card);
        
        void set_playing_card(card_view *card, bool is_response = false);
        void add_modifier(card_view *card);
        bool verify_modifier(card_view *card);

        void handle_auto_respond();
        void handle_auto_targets();

        std::optional<std::string> verify_player_target(target_player_filter filter, player_view *target_player);
        std::optional<std::string> verify_card_target(const effect_holder &args, player_view *player, card_view *card);

        void add_card_target(player_view *player, card_view *card);
        
        int calc_distance(player_view *from, player_view *to);

        const card_view *get_current_card() const;
        const effect_list &get_current_card_effects() const;

        void send_play_card();

        const effect_holder &get_effect_holder(int index);
        int count_selected_cubes(card_view *card);
        std::vector<player_view *> possible_player_targets(target_player_filter filter);
        int get_target_index();
        
    private:
        game_scene *m_game;

        std::vector<card_view *> m_response_highlights;
        std::vector<std::tuple<pocket_type, player_view *, card_view *>> m_picking_highlights;
        raii_editor_stack<sdl::color> m_response_borders;

        effect_flags m_request_flags{};

        card_view *m_last_played_card = nullptr;
        bool m_waiting_confirm = false;

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);
    };

}

#endif