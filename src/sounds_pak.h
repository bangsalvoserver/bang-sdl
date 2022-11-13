#include <filesystem>
#include <array>
#include <map>

#include "sdl_wrap.h"

#include "utils/unpacker.h"

struct wav_deleter {
    void operator()(Uint8 *data) {
        SDL_FreeWAV(data);
    }
};

struct wav_file {
    std::unique_ptr<Uint8[], wav_deleter> buf;
    Uint32 len;
    Uint32 played = 0;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device_id = 0;

    wav_file(resource_view res);
    ~wav_file();

    void play();
};

struct sounds_pak {
public:
    explicit sounds_pak(const std::filesystem::path &base_path);

public:
    void play_sound(std::string_view name);

    static sounds_pak &get() {
        return *s_instance;
    }

private:
    static inline sounds_pak *s_instance = nullptr;

    std::ifstream sounds_pak_data;
    const unpacker<std::ifstream> sounds_resources;

    std::map<std::string, wav_file, std::less<>> wav_cache;
};