#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "game/card_enums.h"
#include "game/game_update.h"

#include "utils/sdl.h"
#include "utils/unpacker.h"

#include "options.h"

#include <filesystem>
#include <vector>
#include <list>

namespace banggame {

    struct card_textures {
    private:
        card_textures(const std::filesystem::path &base_path);

        static inline card_textures *s_instance = nullptr;

        std::ifstream cards_pak_data;
        const unpacker<std::ifstream> card_resources;

        friend class game_scene;

    public:
        sdl::surface card_mask;
        sdl::texture card_border;

        sdl::texture backface_maindeck;
        sdl::texture backface_character;
        sdl::texture backface_role;
        sdl::texture backface_goldrush;

        std::array<sdl::surface, enums::size_v<card_value_type> - 1> value_icons;
        std::array<sdl::surface, enums::size_v<card_suit_type> - 1> suit_icons;

        sdl::surface apply_card_mask(const sdl::surface &source) const;

        sdl::surface get_card_resource(std::string_view name) const {
            return sdl::surface(card_resources[name]);
        }

        static const card_textures &get() {
            return *s_instance;
        }
    };

    struct card_view;

    class card_pile_view : public std::vector<card_view *> {
    protected:
        sdl::point pos;
        int m_width;
        int hflip;

    public:
        uint32_t border_color = 0;

        explicit card_pile_view(int width = 0, bool hflip = false)
            : m_width(width)
            , hflip(hflip ? -1 : 1) {}

        const sdl::point get_pos() const { return pos; }
        int width() const { return m_width; }

        void set_pos(const sdl::point &new_pos);
        virtual sdl::point get_position_of(card_view *card) const;
        void erase_card(card_view *card);
    };

    struct character_pile : card_pile_view {
        sdl::point get_position_of(card_view *card) const override;
    };

    class cube_widget {
    public:
        card_view *owner = nullptr;

        int id;
        sdl::point pos;

        bool animating = false;
        uint32_t border_color = 0;

        cube_widget(int id) : id(id) {}

        void render(sdl::renderer &renderer, bool skip_if_animating = true);
    };

    class card_view : public card_info {
    public:
        std::vector<cube_widget *> cubes;

        float flip_amt = 0.f;
        float rotation = 0.f;

        bool animating = false;
        
        bool known = false;
        card_pile_view *pile = nullptr;

        bool inactive = false;

        uint32_t border_color = 0;

        void set_pos(const sdl::point &pos);
        const sdl::point &get_pos() const {
            return m_pos;
        }

        const sdl::rect &get_rect() const {
            return m_rect;
        }

        void render(sdl::renderer &renderer, bool skip_if_animating = true);

        sdl::texture texture_front;
        sdl::texture texture_front_scaled;

        const sdl::texture *texture_back = nullptr;
        
        void make_texture_front();

    private:
        sdl::point m_pos;
        sdl::rect m_rect;
    };

    struct role_card : card_view {
        player_role role = player_role::unknown;

        void make_texture_front();
    };
}

#endif