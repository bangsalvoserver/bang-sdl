#include "utils/sdl.h"

#include "server/net_options.h"

#include "manager.h"

constexpr int window_width = 900;
constexpr int window_height = 700;

DECLARE_RESOURCE(bang_png)

extern "C" __declspec(dllexport) long __stdcall entrypoint(const char *base_path) {
    sdl::initializer sdl_init(SDL_INIT_VIDEO);
    sdl::ttf_initializer sdl_ttf_init;
    sdl::img_initializer sdl_img_init(IMG_INIT_PNG | IMG_INIT_JPG);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    sdl::window window(_("BANG_TITLE").c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);
    sdl::surface window_icon(GET_RESOURCE(bang_png));
    SDL_SetWindowIcon(window.get(), window_icon.get());

    sdl::renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);

    game_manager mgr{base_path};
    mgr.resize(window_width, window_height);

    sdl::event event;
    bool quit = false;

    using frames = std::chrono::duration<int64_t, std::ratio<1, banggame::fps>>;
    auto next_frame = std::chrono::high_resolution_clock::now() + frames{0};

    while (!quit) {
        next_frame += frames{1};

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
        
        std::this_thread::sleep_until(next_frame);
    }

    return 0;
}