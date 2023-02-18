#ifndef __TARGET_FINDER_H__
#define __TARGET_FINDER_H__

#include "player.h"

#include "game/holders.h"
#include "utils/raii_editor.h"

#include <vector>

namespace banggame {

    class game_scene;

    enum class target_mode {
        start,
        modifier,
        target,
        equip,
        finish
    };

    struct target_status {
        card_view *m_playing_card = nullptr;
        target_list m_targets;
        modifier_list m_modifiers;
        raii_editor_stack<sdl::color> m_target_borders;
        target_mode m_mode = target_mode::start;

        card_view *get_current_card() const;
        target_list &get_current_target_list();
        const target_list &get_current_target_list() const;
        bool has_modifier(card_modifier_type type) const;
    };

    struct request_status {
        std::vector<card_view *> m_response_cards;
        std::vector<card_view *> m_picking_cards;
        raii_editor_stack<sdl::color> m_response_borders;
        effect_flags m_request_flags{};
        bool m_response = false;
    };

    class target_finder : private target_status, private request_status {
    public:
        target_finder(game_scene *parent) : m_game(parent) {}

        bool is_card_clickable() const;
        bool can_confirm() const;
        
        bool is_bangcard(card_view *card) const;
        int calc_distance(player_view *from, player_view *to) const;

        bool is_playing_card(card_view *card) const { return m_playing_card == card; }
        bool finished() const { return m_mode == target_mode::finish; }
        card_view *get_last_played_card() const { return m_last_played_card; }

        bool can_respond_with(card_view *card) const;
        bool can_pick_card(pocket_type pocket, player_view *player, card_view *card) const;
        bool can_play_in_turn(pocket_type pocket, player_view *player, card_view *card) const;
    
    public:
        void set_picking_border(card_view *card, sdl::color color);
        void set_response_cards(const request_status_args &args);
        void set_play_cards(const status_ready_args &args);
        
        void handle_auto_respond();

        void update_last_played_card();
        void clear_status();
        void clear_targets();

        void on_click_card(pocket_type pocket, player_view *player, card_view *card);
        bool on_click_player(player_view *player);
        
        void send_prompt_response(bool response);
    
    private:
        void set_playing_card(card_view *card);
        bool playable_with_modifiers(card_view *card) const;

        const char *check_player_filter(target_player_filter filter, player_view *target_player);
        const char *check_card_filter(target_card_filter filter, card_view *target_card);

        int count_selected_cubes(card_view *card);
        bool add_selected_cube(card_view *card, int ncubes);

        void add_card_target(player_view *player, card_view *card);
        void handle_auto_targets();

        void send_pick_card(pocket_type pocket, player_view *player, card_view *card);
        void send_play_card();
        
    private:
        game_scene *m_game;

        card_view *m_last_played_card = nullptr;

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);
    };

}

#endif