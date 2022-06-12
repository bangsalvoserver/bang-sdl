#include "sdl_wrap.h"

#include "game/net_options.h"

#include "manager.h"
#include "media_pak.h"

#include "bangclient_export.h"

#ifdef WIN32
    #define STDCALL __stdcall
#else
    #define STDCALL
#endif

constexpr int window_width = 900;
constexpr int window_height = 700;
constexpr int max_fps = 300;

extern "C" BANGCLIENT_EXPORT long STDCALL entrypoint(const char *base_path) {
    sdl::initializer sdl_init(SDL_INIT_VIDEO);
    sdl::ttf_initializer sdl_ttf_init;
    sdl::img_initializer sdl_img_init(IMG_INIT_PNG | IMG_INIT_JPG);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    sdl::window window(_("BANG_TITLE").c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_RESIZABLE);

    sdl::renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
    
    media_pak resources{base_path, renderer};
    SDL_SetWindowIcon(window.get(), media_pak::get().icon_bang.get());

    asio::io_context ctx;
    auto work_guard = asio::make_work_guard(ctx.get_executor());

    client_manager mgr{window, renderer, ctx, base_path};

    sdl::event event;
    bool quit = false;

    using clock = std::chrono::steady_clock;
    using frames = std::chrono::duration<int64_t, std::ratio<1, max_fps>>;

    auto next_frame = clock::now() + frames{0};
    auto last_tick = clock::now();

    while (!quit) {
        next_frame += frames{1};

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    mgr.refresh_layout();
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

        ctx.poll();

        auto next_tick = clock::now();
        mgr.tick(next_tick - last_tick);
        last_tick = next_tick;

        mgr.render(renderer);
        SDL_RenderPresent(renderer.get());
        
        std::this_thread::sleep_until(next_frame);
    }

    return 0;
}