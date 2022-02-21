#ifndef __GLOBAL_RESOURCES_H__
#define __GLOBAL_RESOURCES_H__

#include "utils/sdl.h"

class global_resources {
public:
    sdl::surface icon_bang;
    sdl::texture texture_background;
    sdl::texture icon_checkbox;
    sdl::texture icon_default_user;
    sdl::texture icon_disconnected;
    sdl::texture icon_loading;
    sdl::texture icon_owner;

    static const global_resources &get() {
        return *s_instance;
    }

public:
    global_resources();

private:
    static inline global_resources *s_instance = nullptr;
};

#endif