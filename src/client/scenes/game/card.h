#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "common/card_enums.h"
#include "common/game_update.h"

#include "utils/sdl.h"

#include <concepts>
#include <vector>

namespace banggame {

    struct card_pile_view : std::vector<int> {
        SDL_Point pos;
        int xoffset;

        card_pile_view(int xoffset = 30) : xoffset(xoffset) {}

        auto find(int card_id) const {
            return std::ranges::find(*this, card_id);
        }

        SDL_Point get_position(int card_id) const {
            return SDL_Point{(int)(pos.x + xoffset *
                (std::ranges::distance(begin(), find(card_id)) - (size() - 1) * .5f)),
                pos.y};
        }

        void erase_card(int card_id) {
            if (auto it = find(card_id); it != end()) {
                erase(it);
            }
        }
    };

    class card_widget_base {
    public:
        SDL_Point pos;

        float flip_amt = 0.f;
        float rotation = 0.f;

        static constexpr int card_width = 70;

        int id;
        std::string name;
        std::string image;
        std::vector<card_target_data> targets;

        const SDL_Rect &get_rect() const {
            return m_rect;
        }

    private:
        SDL_Rect m_rect;
        
    protected:
        void render(sdl::renderer &renderer, sdl::texture &front);
    };

    template<typename T>
    struct card_widget : card_widget_base {
        sdl::texture texture_front;
        static inline sdl::texture texture_back;

        void render(sdl::renderer &renderer) {
            if (flip_amt > 0.5f && texture_front) card_widget_base::render(renderer, texture_front);
            else if (texture_back) card_widget_base::render(renderer, texture_back);
        }
    };

    struct card_view : card_widget<card_view> {
        bool known = false;
        bool inactive = false;

        card_pile_view *pile = nullptr;

        card_suit_type suit;
        card_value_type value;
        card_color_type color;

        void make_texture_front();
        static void make_texture_back();
    };

    struct character_card : card_widget<character_card> {
        character_type type;

        void make_texture_front();
        static void make_texture_back();
    };

    struct role_card : card_widget<role_card> {
        player_role role = player_role::unknown;

        void make_texture_front();
        static void make_texture_back();
    };

    struct player_view {
        static constexpr float one_hp_size = 20.f;
        int hp = 0;

        card_pile_view hand;
        card_pile_view table;

        character_card m_character;
        character_card m_hp_marker;
        role_card m_role;

        void set_position(SDL_Point pos, bool flipped = false);

        void set_hp_marker_position(float hp);

        void render(sdl::renderer &renderer);
    };
}

#endif