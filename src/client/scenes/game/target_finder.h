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

    struct target_pair {
        player_view *player;
        card_widget *card;
    };

    struct target_status {
        card_widget *m_playing_card = nullptr;
        card_widget *m_modifier = nullptr;

        std::vector<std::vector<target_pair>> m_targets;
        std::vector<cube_widget *> m_selected_cubes;

        play_card_flags m_flags = enums::flags_none<play_card_flags>;

        std::optional<card_view> m_virtual;
    };

    class target_finder : private target_status {
    public:
        target_finder(game_scene *parent) : m_game(parent) {}
        
        void render(sdl::renderer &renderer);

        play_card_flags get_flags() const {
            return m_flags;
        }

        void on_click_main_deck();
        void on_click_selection_card(card_view *card);
        void on_click_shop_card(card_view *card);
        void on_click_table_card(player_view *player, card_view *card);
        void on_click_hand_card(player_view *player, card_view *card);
        void on_click_character(player_view *player, character_card *card);
        void on_click_player(player_view *player);

        void on_click_sell_beer();
        void on_click_discard_black();

        void handle_virtual_card(const virtual_card_update &args);

        void clear_targets();
    
    private:
        bool verify_modifier(card_widget *card);

        void handle_auto_targets();
        void add_card_target(target_pair target);
        void add_character_target(target_pair target);
        void add_player_targets( const std::vector<target_pair> &targets);

        std::vector<card_target_data> &get_current_card_targets();

        void send_play_card();
        
    private:
        game_scene *m_game;

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);
    };

}

#endif