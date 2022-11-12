#include <filesystem>
#include <array>
#include <map>

#include "sdl_wrap.h"

#include "utils/unpacker.h"

struct wav_file {
    Uint8 *buf;
    Uint32 len;
    SDL_AudioDeviceID device_id;

    wav_file(resource_view res);
    ~wav_file();

    wav_file(const wav_file &other) = delete;
    wav_file(wav_file &&other) = delete;
    wav_file &operator = (const wav_file &other) = delete;
    wav_file &operator = (wav_file &&other) = delete;

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