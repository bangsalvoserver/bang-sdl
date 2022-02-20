#include "global_resources.h"

#include "utils/resource.h"

DECLARE_RESOURCE(bang_png)
DECLARE_RESOURCE(background_png)
DECLARE_RESOURCE(icon_checkbox_png)
DECLARE_RESOURCE(icon_default_user_png)
DECLARE_RESOURCE(icon_disconnected_png)
DECLARE_RESOURCE(icon_loading_png)

global_resources::global_resources() {
    s_instance = this;

    icon_bang = sdl::surface(GET_RESOURCE(bang_png));
    texture_background = sdl::texture(sdl::surface(GET_RESOURCE(background_png)));
    icon_checkbox = sdl::texture(sdl::surface(GET_RESOURCE(icon_checkbox_png)));
    icon_default_user = sdl::texture(sdl::surface(GET_RESOURCE(icon_default_user_png)));
    icon_disconnected = sdl::texture(sdl::surface(GET_RESOURCE(icon_disconnected_png)));
    icon_loading = sdl::texture(sdl::surface(GET_RESOURCE(icon_loading_png)));
}