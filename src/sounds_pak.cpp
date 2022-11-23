#include "sounds_pak.h"

#include <thread>

wav_file::wav_file(resource_view res) {
    Uint8 *ptr;
    SDL_LoadWAV_RW(SDL_RWFromConstMem(res.data, int(res.length)), 0, &spec, &ptr, &len);
    buf.reset(ptr);
    if (!buf) {
        throw sdl::error(SDL_GetError());
    }

    spec.userdata = this;
    spec.callback = [](void *userdata, Uint8 *stream, int len) {
        auto &self = *static_cast<wav_file *>(userdata);

        SDL_memset(stream, 0, len);
        len = std::min<int>(len, self.len - self.played);
        if (len > 0) {
            SDL_MixAudioFormat(stream, self.buf.get() + self.played, self.spec.format, len, self.volume);
            self.played += len;
        }
    };

    device_id = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
}

wav_file::~wav_file() {
    if (played < len) {
        SDL_CloseAudioDevice(device_id);
    }
}

void wav_file::play(float vol) {
    played = 0;
    volume = int(SDL_MIX_MAXVOLUME * std::clamp(vol, 0.f, 1.f));
    SDL_PauseAudioDevice(device_id, 0);
}

sounds_pak::sounds_pak(const std::filesystem::path &base_path)
    : sounds_pak_data(ifstream_or_throw(base_path / "sounds.pak"))
    , sounds_resources(sounds_pak_data)
{   
    s_instance = this;
}

void sounds_pak::play_sound(std::string_view name, float volume) {
    auto it = wav_cache.find(name);
    if (it == wav_cache.end()) {
        it = wav_cache.emplace_hint(it, name, sounds_resources[name]);
    }

    it->second.play(volume);
}