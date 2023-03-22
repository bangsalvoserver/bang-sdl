#ifndef __TARGET_FINDER_H__
#define __TARGET_FINDER_H__

#include "player.h"

#include "cards/holders.h"
#include "cards/effect_context.h"
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
        effect_context m_context;
        raii_editor_stack<sdl::color> m_target_borders;
        target_mode m_mode = target_mode::start;

        card_view *get_current_card() const;
        target_list &get_current_target_list();
        const target_list &get_current_target_list() const;
    };

    struct request_status {
        card_modifier_tree m_play_cards;
        std::vector<card_view *> m_pick_cards;
        raii_editor_stack<sdl::color> m_request_borders;
        card_view *m_request_origin_card = nullptr;
        player_view *m_request_origin = nullptr;
        player_view *m_request_target = nullptr;
        bool m_auto_select = false;
        bool m_response = false;
    };

    class target_finder : private target_status, private request_status {
    public:
        target_finder(game_scene *parent) : m_game(parent) {}

        bool is_card_clickable() const;
        bool is_card_selected() const;
        bool can_confirm() const;
        
        int calc_distance(player_view *from, player_view *to) const;

        bool is_playing_card(card_view *card) const { return m_playing_card == card; }
        bool finished() const { return m_mode == target_mode::finish; }

        card_view *get_request_origin_card() const { return m_request_origin_card; }
        player_view *get_request_origin() const { return m_request_origin; }
        player_view *get_request_target() const { return m_request_target; }
    
    public:
        void set_response_cards(const request_status_args &args);
        void set_play_cards(const status_ready_args &args);
        
        void handle_auto_select();

        void clear_status();
        void clear_targets();

        void on_click_card(pocket_type pocket, player_view *player, card_view *card);
        bool on_click_player(player_view *player);
        
        void send_prompt_response(bool response);
    
    private:
        void add_pick_border(card_view *card, sdl::color color);

        void select_playing_card(card_view *card);
        void select_equip_card(card_view *card);

        const card_modifier_tree &get_current_tree() const;
        bool can_play_card(card_view *card) const;

        void add_modifier_context(card_view *mod_card, player_view *target_player, card_view *target_card);

        bool can_pick_card(pocket_type pocket, player_view *player, card_view *card) const;
        
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

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);
    };

}

#endif