#ifndef __SCENE_BASE_H__
#define __SCENE_BASE_H__

#include "net/messages.h"

#include "sdl_wrap.h"

#include "../widgets/button.h"
#include "../widgets/textbox.h"

class client_manager;

template<utils::fixed_string E> requires banggame::server_message_type<E>
struct message_handler {
    virtual void handle_message(utils::tag<E>) = 0;
};

template<utils::fixed_string E> requires banggame::server_message_type<E>
using server_message_value_type = utils::tagged_variant_value_type<banggame::server_message, utils::tag<E>>

template<utils::fixed_string E> requires (banggame::server_message_type<E> && !std::is_void_v<server_message_value_type<E>>)
struct message_handler<E> {
    virtual void handle_message(utils::tag<E>, const server_message_value_type<E> &args) = 0;
};

class scene_base {
public:
    scene_base(client_manager *parent) : parent(parent) {}
    virtual ~scene_base() = default;
    
    virtual void refresh_layout() = 0;

    virtual void tick(duration_type time_elapsed) {}
    
    virtual void render(sdl::renderer &renderer) = 0;
    
    virtual void handle_event(const sdl::event &event) {}

    client_manager *manager() const {
        return parent;
    }

protected:
    client_manager *parent;
};

#endif