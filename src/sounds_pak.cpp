#include "sounds_pak.h"

#include <thread>

wav_file::wav_file(resource_view res) {
    SDL_AudioSpec spec;
    SDL_LoadWAV_RW(SDL_RWFromConstMem(res.data, static_cast<int>(res.length)), 0, &spec, &buf, &len);
    if (!buf) {
        throw sdl::error(SDL_GetError());
    }

    device_id = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
}

wav_file::~wav_file() {
    SDL_CloseAudioDevice(device_id);
    SDL_FreeWAV(buf);
}

void wav_file::play() {
    SDL_ClearQueuedAudio(device_id);
    SDL_QueueAudio(device_id, buf, len);
    SDL_PauseAudioDevice(device_id, 0);
}

sounds_pak::sounds_pak(const std::filesystem::path &base_path)
    : sounds_pak_data(ifstream_or_throw(base_path / "sounds.pak"))
    , sounds_resources(sounds_pak_data)
{   
    s_instance = this;
}

void sounds_pak::play_sound(std::string_view name) {
    auto it = wav_cache.find(name);
    if (it == wav_cache.end()) {
        it = wav_cache.emplace_hint(it, name, sounds_resources[name]);
    }

    it->second.play();
}