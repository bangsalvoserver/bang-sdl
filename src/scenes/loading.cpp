#include "loading.h"

#include "../manager.h"
#include "../media_pak.h"

loading_scene::loading_scene(client_manager *parent, const std::string &text, const std::string &address)
    : scene_base(parent)
    , m_loading_text(text)
    , m_cancel_btn(_("BUTTON_CANCEL"), [parent]{
        parent->add_chat_message(message_type::server_log, _("ERROR_CONNECTION_CANCELED"));
        parent->disconnect();
    })
    , m_address(address) {}

void loading_scene::handle_message(SRV_TAG(client_accepted), const banggame::client_accepted_args &args) {
    parent->client_accepted(args, m_address);
}

void loading_scene::refresh_layout() {
    const auto win_rect = parent->get_rect();
    
    sdl::rect rect = m_loading_text.get_rect();
    rect.x = (win_rect.w - rect.w) / 2;
    rect.y = (win_rect.h - rect.h) / 2;
    m_loading_text.set_rect(rect);

    m_cancel_btn.set_rect(sdl::rect{
        (win_rect.w - 100) / 2,
        rect.y + rect.h + 10,
        100, 25
    });

    m_loading_rect = media_pak::get().icon_loading.get_rect();
    m_loading_rect.x = (win_rect.w - m_loading_rect.w) / 2;
    m_loading_rect.y = rect.y - m_loading_rect.h - 10;
}

void loading_scene::tick(duration_type time_elapsed) {
    static constexpr float rotation_speed = 600.f;
    m_loading_rotation += rotation_speed * time_elapsed / std::chrono::seconds{1};
}

void loading_scene::render(sdl::renderer &renderer) {
    m_loading_text.render(renderer);
    m_cancel_btn.render(renderer);

    media_pak::get().icon_loading.render_ex(renderer, m_loading_rect, sdl::render_ex_options{ .angle = m_loading_rotation });
}