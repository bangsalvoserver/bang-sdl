#ifndef __SCENE_GAME_H__
#define __SCENE_GAME_H__

#include "../scene_base.h"

#include "card.h"

#include "common/responses.h"

#include <list>

namespace banggame {

    struct player_view_cards {
        player_view player;

        std::vector<int> hand;
        std::vector<int> table;
    };

    struct card_view_location {
        card_view card;
        card_pile_type pile = card_pile_type::main_deck;
        int pile_value = 0;
        
        sdl::texture texture_front;
        static inline sdl::texture texture_back;

        SDL_Point pos;
        float flip_amt = 0.f;
        float rotation = 0.f;
    };

    struct response_view {
        response_type type = response_type::none;
        int origin_id;
        int target_id;
    };

    struct card_move_animation {
        std::map<card_view_location *, std::pair<SDL_Point, SDL_Point>> data;

        void add_move_card(card_view_location &c, SDL_Point pt) {
            auto [it, inserted] = data.try_emplace(&c, c.pos, pt);
            if (!inserted) {
                it->second.second = pt;
            }
        }

        void do_animation(float amt) {
            for (auto &[card, pos] : data) {
                card->pos.x = std::lerp(pos.first.x, pos.second.x, amt);
                card->pos.y = std::lerp(pos.first.y, pos.second.y, amt);
            }
        }
    };

    struct card_flip_animation {
        card_view_location *card;
        bool flips;

        void do_animation(float amt) {
            card->flip_amt = flips ? amt : 1.f - amt;
        }
    };

    struct card_tap_animation {
        card_view_location *card;
        bool taps;

        void do_animation(float amt) {
            card->rotation = 90.f * (taps ? amt : 1.f - amt);
        }
    };

    struct player_hp_animation {
        player_view *player;

        int hp_from;
        int hp_to;

        void do_animation(float amt) {

        }
    };

    class animation {
    private:
        int duration;
        int elapsed = 0;

        std::variant<card_move_animation, card_flip_animation, card_tap_animation, player_hp_animation> m_anim;

    public:
        animation(int duration, decltype(m_anim) &&anim)
            : duration(duration)
            , m_anim(std::move(anim)) {}

        void tick() {
            ++elapsed;
            std::visit([this](auto &anim) {
                anim.do_animation((float)elapsed / (float)duration);
            }, m_anim);
        }

        bool done() const {
            return elapsed >= duration;
        }
    };

    class game_scene : public scene_base {
    public:
        game_scene(class game_manager *parent);

        SDL_Color bg_color() override {
            return {0x07, 0x63, 0x25, 0xff};
        }
        
        void render(sdl::renderer &renderer, int w, int h) override;
        void handle_event(const SDL_Event &event) override;

        void handle_update(const game_update &update);
        
        void add_chat_message(const lobby_chat_args &args);

    private:
        void handle_update(enums::enum_constant<game_update_type::game_notify>,      game_notify_type args);
        void handle_update(enums::enum_constant<game_update_type::move_card>,        const move_card_update &args);
        void handle_update(enums::enum_constant<game_update_type::show_card>,        const show_card_update &args);
        void handle_update(enums::enum_constant<game_update_type::hide_card>,        const hide_card_update &args);
        void handle_update(enums::enum_constant<game_update_type::tap_card>,         const tap_card_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_own_id>,    const player_own_id_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_hp>,        const player_hp_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_character>, const player_character_update &args);
        void handle_update(enums::enum_constant<game_update_type::player_show_role>, const player_show_role_update &args);
        void handle_update(enums::enum_constant<game_update_type::switch_turn>,      const switch_turn_update &args);
        void handle_update(enums::enum_constant<game_update_type::response_handle>,  const response_handle_update &args);
        void handle_update(enums::enum_constant<game_update_type::response_done>);
        
        void pop_update();

        template<game_action_type T, typename ... Ts>
        void add_action(Ts && ... args);

        std::list<game_update> m_pending_updates;
        std::list<animation> m_animations;
        std::vector<lobby_chat_args> m_messages;

        std::vector<int> main_deck;
        std::vector<int> discard_pile;
        std::vector<int> temp_table;

        std::map<int, card_view_location> m_cards;
        std::map<int, player_view_cards> m_players;

        int m_player_own_id = 0;

        int m_playing_id;

        response_view m_current_response;
    };

}

#endif