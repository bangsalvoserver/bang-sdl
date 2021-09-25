#include <iostream>

#include "utils/sdl.h"
#include "utils/sdlnet.h"

#include "common/options.h"

#include "manager.h"

constexpr int window_width = 640;
constexpr int window_height = 480;
constexpr int fps = 30;

int main(int argc, char **argv) {
    sdl::initializer sdl_init;
    sdl::ttf_initializer sdl_ttf_init;
    sdl::img_initializer sdl_img_init(IMG_INIT_PNG);
    sdlnet::initializer sdl_net_init;

    sdl::window window("Bang!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, 0);
    sdl::renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

    game_manager mgr;

    SDL_Event event;
    bool quit = false;
    while (!quit) {
        mgr.render(renderer, window_width, window_height);
        SDL_RenderPresent(renderer.get());

        mgr.update_net();
        
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                [[fallthrough]];
            default:
                mgr.handle_event(event);
                break;
            }
        }
        SDL_Delay(1000 / fps);
    }

    return 0;
}