#include "sounds_pak.h"

namespace sdl {
    wav_file::wav_file(resource_view res)
        : base(Mix_LoadWAV_RW(SDL_RWFromConstMem(res.data, int(res.length)), 0)) {
        if (!*this) {
            throw error(fmt::format("Error: cannot load wav: {}", Mix_GetError()));
        }
    }
}

sounds_pak::sounds_pak(const std::filesystem::path &base_path)
    : sounds_pak_data(ifstream_or_throw(base_path / "sounds.pak"))
    , sounds_resources(sounds_pak_data)
{   
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) < 0) {
        throw sdl::error(fmt::format("Error: could not initialize mixer: {}", Mix_GetError()));
    }
}

sounds_pak::~sounds_pak() {
    Mix_CloseAudio();
}

void sounds_pak::play_sound(std::string_view name, float volume) {
    if (sounds_resources.contains(name)) {
        auto it = wav_cache.find(name);
        if (it == wav_cache.end()) {
            it = wav_cache.emplace_hint(it, name, sounds_resources[name]);
        }

        Mix_Chunk *chunk = it->second.get();
        Mix_VolumeChunk(chunk, int(volume * MIX_MAX_VOLUME));
        Mix_PlayChannel(-1, chunk, 0);
    }
}