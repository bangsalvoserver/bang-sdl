#ifndef __CLIENT_CARD_H__
#define __CLIENT_CARD_H__

#include "game/card_enums.h"
#include "game/game_update.h"

#include "sdl_wrap.h"
#include "utils/unpacker.h"

#include "options.h"

#include "../widgets/stattext.h"

#include <filesystem>
#include <vector>
#include <memory>

namespace banggame {

    struct card_textures {
    private:
        card_textures(const std::filesystem::path &base_path, sdl::renderer &renderer);

        static inline card_textures *s_instance = nullptr;

        std::ifstream cards_pak_data;
        const unpacker<std::ifstream> card_resources;

        friend class game_scene;

    public:
        sdl::surface card_mask;
        sdl::texture card_border;

        std::array<sdl::texture, enums::num_members_v<card_deck_type>> backfaces;

        std::array<sdl::surface, enums::num_members_v<card_rank> - 1> rank_icons;
        std::array<sdl::surface, enums::num_members_v<card_suit> - 1> suit_icons;

        sdl::texture make_backface_texture(sdl::renderer &renderer, card_deck_type type);
        sdl::surface apply_card_mask(const sdl::surface &source) const;

        sdl::surface get_card_resource(std::string_view name) const {
            return sdl::surface(card_resources[name]);
        }

        static const card_textures &get() {
            return *s_instance;
        }
    };

    DEFINE_ENUM_FLAGS(render_flags,
        (no_skip_animating)
        (no_draw_border)
    )

    class pocket_view;
    class card_view;

    struct cube_widget {
        sdl::point pos;

        bool animating = false;
        sdl::color border_color{};

        void render(sdl::renderer &renderer, render_flags flags = {});
    };

    struct cube_pile_base : std::vector<std::unique_ptr<cube_widget>> {
        virtual void set_pos(sdl::point pos);

        virtual sdl::point get_pos() const = 0;
        virtual sdl::point get_offset(cube_widget *cube) const = 0;

        void render(sdl::renderer &renderer) {
            for (auto &cube : *this) {
                cube->render(renderer);
            }
        }
    };

    struct card_cube_pile : cube_pile_base {
        card_view *owner;

        card_cube_pile(card_view *owner) : owner(owner) {}

        sdl::point get_pos() const override;
        sdl::point get_offset(cube_widget *cube) const override;
    };

    struct table_cube_pile : cube_pile_base {
        sdl::point m_pos;

        void set_pos(sdl::point pos) override;
        sdl::point get_pos() const override { return m_pos; }
        sdl::point get_offset(cube_widget *cube) const override;
    };

    class card_view : public card_data {
    public:
        card_cube_pile cubes{this};

        float flip_amt = 0.f;
        float rotation = 0.f;

        bool animating = false;
        
        bool known = false;
        pocket_view *pocket = nullptr;

        bool inactive = false;

        sdl::color border_color{};

        void set_pos(const sdl::point &pos);
        const sdl::point &get_pos() const {
            return m_pos;
        }

        const sdl::rect &get_rect() const {
            return m_rect;
        }

        void render(sdl::renderer &renderer, render_flags flags = {});

        sdl::texture texture_front;
        sdl::texture texture_front_scaled;

        sdl::texture_ref texture_back;
        
        void make_texture_front(sdl::renderer &renderer);

    private:
        sdl::point m_pos;
        sdl::rect m_rect;
    };

    struct role_card : card_view {
        player_role role = player_role::unknown;

        void make_texture_front(sdl::renderer &renderer);
    };
    
    class pocket_view {
    protected:
        std::vector<card_view *> m_cards;
        sdl::point m_pos;

    public:
        sdl::point get_pos() const { return m_pos; }
        virtual void set_pos(const sdl::point &pos);

        size_t size() const { return m_cards.size(); }
        bool empty() const { return m_cards.empty(); }

        auto begin() { return m_cards.begin(); }
        auto begin() const { return m_cards.begin(); }
        auto end() { return m_cards.end(); }
        auto end() const { return m_cards.end(); }
        auto rbegin() { return m_cards.rbegin(); }
        auto rbegin() const { return m_cards.rbegin(); }
        auto rend() { return m_cards.rend(); }
        auto rend() const { return m_cards.rend(); }

        auto &front() { return m_cards.front(); }
        const auto &front() const { return m_cards.front(); }
        auto &back() { return m_cards.back(); }
        const auto &back() const { return m_cards.back(); }
    
    public:
        virtual void render_first(sdl::renderer &renderer, int ncards);
        virtual void render_last(sdl::renderer &renderer, int ncards);

        virtual void update_card(card_view *card) {}
        virtual void add_card(card_view *card);
        virtual void erase_card(card_view *card);
        virtual void clear();

        virtual sdl::point get_offset(card_view *card) const { return {0, 0}; }
        virtual bool wide() const { return false; }
        virtual card_view *find_card_at(sdl::point point);

        virtual void render(sdl::renderer &renderer);
    };

    class point_pocket_view : public pocket_view {
    public:
        sdl::color border_color{};
        
        card_view *find_card_at(sdl::point point);

        void render_border(sdl::renderer &renderer);
        
        void render(sdl::renderer &renderer) override {
            render_border(renderer);
            pocket_view::render(renderer);
        }

        void render_first(sdl::renderer &renderer, int ncards) override {
            render_border(renderer);
            pocket_view::render_first(renderer, ncards);
        }

        void render_last(sdl::renderer &renderer, int ncards) override {
            render_border(renderer);
            pocket_view::render_last(renderer, ncards);
        }
    };

    class counting_pocket : public point_pocket_view {
    private:
        widgets::stattext m_count_text;
        void update_count();

    public:
        void render_count(sdl::renderer &renderer);

        void render(sdl::renderer &renderer) override {
            point_pocket_view::render(renderer);
            render_count(renderer);
        }

        void render_first(sdl::renderer &renderer, int ncards) override {
            point_pocket_view::render_first(renderer, ncards);
            render_count(renderer);
        }

        void render_last(sdl::renderer &renderer, int ncards) override {
            point_pocket_view::render_last(renderer, ncards);
            render_count(renderer);
        }

        void add_card(card_view *card) override {
            point_pocket_view::add_card(card);
            update_count();
        }

        void erase_card(card_view *card) override {
            card->pocket = nullptr;
            point_pocket_view::erase_card(card);
            update_count();
        }

        void clear() override {
            point_pocket_view::clear();
            update_count();
        }
    };

    struct wide_pocket : pocket_view {
        int width;
        explicit wide_pocket(int width) : width(width) {}

        bool wide() const override { return true; }
        sdl::point get_offset(card_view *card) const override;
    };

    struct flipped_pocket : wide_pocket {
        using wide_pocket::wide_pocket;
        sdl::point get_offset(card_view *card) const override;
    };

    struct character_pile : pocket_view {
        sdl::point get_offset(card_view *card) const override;
    };

}

#endif