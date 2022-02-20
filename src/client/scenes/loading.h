#ifndef __LOADING_H__
#define __LOADING_H__

#include "scene_base.h"

class loading_scene : public scene_base {
public:
    loading_scene(class game_manager *parent);

    void init(const std::string &address);

    void resize(int width, int height) override;

    void render(sdl::renderer &renderer) override;

private:
    widgets::stattext m_loading_text;
    widgets::button m_cancel_btn;

    sdl::texture m_loading_tex;
    sdl::rect m_loading_rect;
    float m_loading_rotation = 0.f;
};

#endif