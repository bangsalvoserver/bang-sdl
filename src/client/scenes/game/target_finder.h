#ifndef __TARGET_FINDER_H__
#define __TARGET_FINDER_H__

#include "card.h"

#include "common/requests.h"

#include <vector>

namespace banggame {

    class game_scene;

    constexpr bool is_timer_request(request_type type) {
        constexpr auto lut = []<request_type ... Es>(enums::enum_sequence<Es ...>) {
            return std::array{ timer_request<Es> ... };
        }(enums::make_enum_sequence<request_type>());
        return lut[enums::indexof(type)];
    }

    constexpr bool is_picking_request(request_type type) {
        constexpr auto lut = []<request_type ... Es>(enums::enum_sequence<Es ...>) {
            return std::array{ picking_request<Es> ... };
        }(enums::make_enum_sequence<request_type>());
        return lut[enums::indexof(type)];
    };

    class target_finder {
    public:
        target_finder(game_scene *parent) : m_game(parent) {}
        
        void render(sdl::renderer &renderer);

        play_card_flags get_flags() const {
            return m_play_card_args.flags;
        }

        void on_click_main_deck();
        void on_click_selection_card(card_view *card);
        void on_click_shop_card(card_view *card);
        void on_click_table_card(int player_id, card_view *card);
        void on_click_hand_card(int player_id, card_view *card);
        void on_click_character(int player_id, character_card *card);
        void on_click_player(int player_id);

        void on_click_sell_beer();
        void on_click_discard_black();

        void handle_virtual_card(const virtual_card_update &args);

        void clear_targets();
    
    private:
        bool verify_modifier(card_widget *card);

        void handle_auto_targets(bool is_response);
        void add_card_target(bool is_response, const target_card_id &target);
        void add_character_target(bool is_response, const target_card_id &target);
        void add_player_targets(bool is_response, const std::vector<target_player_id> &targets);

        std::vector<card_target_data> &get_current_card_targets(bool is_response);
        
    private:
        game_scene *m_game;

        play_card_args m_play_card_args;
        std::vector<card_widget *> m_highlights;
        std::optional<card_view> m_virtual;

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);
    };

}

#endif