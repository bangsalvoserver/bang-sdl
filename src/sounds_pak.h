#include <filesystem>
#include <array>
#include <map>

#include <SDL2/SDL_mixer.h>
#include "sdl_wrap.h"

#include "utils/unpacker.h"

namespace sdl {
    struct chunk_deleter {
        void operator()(Mix_Chunk *chunk) {
            Mix_FreeChunk(chunk);
        }
    };

    class wav_file: public std::unique_ptr<Mix_Chunk, chunk_deleter> {
        using base = std::unique_ptr<Mix_Chunk, chunk_deleter>;

    public:
        wav_file(resource_view res);
    };
}

struct sounds_pak {
public:
    explicit sounds_pak(const std::filesystem::path &base_path);
    ~sounds_pak();

public:
    void play_sound(std::string_view name, float volume = 1.f);

private:
    std::ifstream sounds_pak_data;
    const unpacker<std::ifstream> sounds_resources;

    std::map<std::string, sdl::wav_file, std::less<>> wav_cache;
};