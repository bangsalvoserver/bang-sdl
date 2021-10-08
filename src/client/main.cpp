#include <iostream>

#include "utils/sdl.h"
#include "utils/sdlnet.h"

#include "common/options.h"

#include "manager.h"

constexpr int window_width = 800;
constexpr int window_height = 600;

int main(int argc, char **argv) {
    sdl::initializer sdl_init;
    sdl::ttf_initializer sdl_ttf_init;
    sdl::img_initializer sdl_img_init(IMG_INIT_PNG);
    sdlnet::initializer sdl_net_init;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    sdl::window window("Bang!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);
    sdl::renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

    game_manager mgr(SDL_GetBasePath() + std::string("config.json"));
    mgr.resize(window_width, window_height);

    sdl::event event;
    bool quit = false;
    while (!quit) {
        mgr.render(renderer);
        SDL_RenderPresent(renderer.get());

        mgr.update_net();
        
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    mgr.resize(event.window.data1, event.window.data2);
                }
                break;
            case SDL_QUIT:
                quit = true;
                break;
            default:
                mgr.handle_event(event);
                break;
            }
        }
        SDL_Delay(1000 / banggame::fps);
    }

    return 0;
}