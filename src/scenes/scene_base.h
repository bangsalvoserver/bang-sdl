#ifndef __SCENE_BASE_H__
#define __SCENE_BASE_H__

#include "game/messages.h"

#include "sdl_wrap.h"

#include "../widgets/button.h"
#include "../widgets/textbox.h"

class client_manager;

template<banggame::server_message_type E>
struct message_handler {
    virtual void handle_message(enums::enum_tag_t<E>) = 0;
};

template<banggame::server_message_type E> requires enums::value_with_type<E>
struct message_handler<E> {
    virtual void handle_message(enums::enum_tag_t<E>, const enums::enum_type_t<E> &args) = 0;
};

class scene_base {
public:
    scene_base(client_manager *parent) : parent(parent) {}
    virtual ~scene_base() = default;
    
    virtual void refresh_layout() = 0;

    virtual void tick(duration_type time_elapsed) {}
    
    virtual void render(sdl::renderer &renderer) = 0;
    
    virtual void handle_event(const sdl::event &event) {}

protected:
    client_manager *parent;
};

#endif