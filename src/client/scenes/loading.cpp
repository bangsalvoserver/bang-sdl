#include "loading.h"

#include "../manager.h"
#include "../media_pak.h"

loading_scene::loading_scene(client_manager *parent, const std::string &address)
    : scene_base(parent)
    , m_loading_text(_("CONNECTING_TO", address))
    , m_cancel_btn(_("BUTTON_CANCEL"), [parent]{
        parent->disconnect(_("ERROR_CONNECTION_CANCELED"));
    }) {}

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

    m_loading_rect = media_pak::get().icon_loading.get_rect();
    m_loading_rect.x = (width - m_loading_rect.w) / 2;
    m_loading_rect.y = rect.y - m_loading_rect.h - 10;
}

void loading_scene::render(sdl::renderer &renderer) {
    m_loading_text.render(renderer);
    m_cancel_btn.render(renderer);

    SDL_RenderCopyEx(renderer.get(), media_pak::get().icon_loading.get_texture(renderer), nullptr,
        &m_loading_rect, m_loading_rotation, nullptr, SDL_FLIP_NONE);
    m_loading_rotation += 10.f;
}