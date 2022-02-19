#include "loading.h"

#include "../manager.h"

loading_scene::loading_scene(game_manager *parent)
    : scene_base(parent)
    , m_cancel_btn(_("BUTTON_CANCEL"), std::bind(&game_manager::disconnect, parent, _("ERROR_CONNECTION_CANCELED"))) {}

void loading_scene::init(const std::string &address) {
    m_loading_text.redraw(_("CONNECTING_TO", address));
    resize(parent->width(), parent->height());
}

void loading_scene::resize(int width, int height) {
    sdl::rect rect = m_loading_text.get_rect();
    rect.x = (width - rect.w) / 2;
    rect.y = (height - rect.h) / 2;
    m_loading_text.set_rect(rect);

    m_cancel_btn.set_rect(sdl::rect{
        (width - 100) / 2,
        rect.y + rect.h + 10,
        100, 25
    });
}

void loading_scene::render(sdl::renderer &renderer) {
    m_loading_text.render(renderer);
    m_cancel_btn.render(renderer);
}