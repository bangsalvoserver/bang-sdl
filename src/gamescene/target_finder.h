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
        std::vector<modifier_pair> m_modifiers;
        bool m_response = false;
        target_mode m_mode = target_mode::start;

        raii_editor_stack<sdl::color> m_target_borders;
    };

    class target_finder : private target_status {
    public:
        target_finder(game_scene *parent) : m_game(parent) {}

        bool can_respond_with(card_view *card) const;

        bool can_pick_card(pocket_type pocket, player_view *player, card_view *card) const;
        bool can_play_in_turn(pocket_type pocket, player_view *player, card_view *card) const;
        bool can_confirm();

        void set_picking_border(card_view *card, sdl::color color);

        void set_response_highlights(const request_status_args &args);
        void clear_status();
        void clear_targets();
        void handle_auto_respond();

        card_view *get_last_played_card() const {
            return m_last_played_card;
        }

        void confirm_play(bool valid);
        bool waiting_confirm() const { return m_mode == target_mode::finish; }

        bool is_card_clickable() const;

        void on_click_card(pocket_type pocket, player_view *player, card_view *card);
        bool on_click_player(player_view *player);

        bool is_playing_card(card_view *card) const {
            return m_playing_card == card;
        }
        
        void send_prompt_response(bool response);
        
        bool is_bangcard(card_view *card) const;
        int calc_distance(player_view *from, player_view *to) const;
    
    private:
        bool set_playing_card(card_view *card, target_mode mode = target_mode::target);
        bool playable_with_modifiers(card_view *card) const;

        void handle_auto_targets();

        const char *check_player_filter(target_player_filter filter, player_view *target_player);
        const char *check_card_filter(target_card_filter filter, card_view *target_card);

        void add_card_target(player_view *player, card_view *card);
        int count_selected_cubes(card_view *card);
        bool add_selected_cube(card_view *card, int ncubes);

        card_view *get_current_card() const;
        const effect_list &get_current_card_effects() const;
        target_list &get_current_target_list();

        void send_pick_card(pocket_type pocket, player_view *player, card_view *card);
        void send_play_card();

        const effect_holder &get_effect_holder(int index);
        int get_target_index();
        
    private:
        game_scene *m_game;

        std::vector<card_view *> m_response_highlights;
        std::vector<card_view *> m_picking_highlights;
        raii_editor_stack<sdl::color> m_response_borders;

        effect_flags m_request_flags{};

        card_view *m_last_played_card = nullptr;

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);
    };

}

#endif