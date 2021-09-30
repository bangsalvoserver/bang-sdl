#ifndef __RESOURCE_H__
#define __RESOURCE_H__

struct resource_view {
    const char *data;
    int length;
};

#ifndef NO_LINK_RESOURCES

#define RESOURCE_NAME(name) __resource__##name
#define RESOURCE_LENGTH(name) __resource__##name##_length
#define DECLARE_RESOURCE(name) extern const char RESOURCE_NAME(name)[]; extern const int RESOURCE_LENGTH(name);
#define GET_RESOURCE(name) resource_view{RESOURCE_NAME(name), RESOURCE_LENGTH(name)}

#else

#include <vector>
#include <fstream>
#include <iterator>
#include <SDL2/SDL.h>

struct resource : std::vector<char> {
    resource(const std::string &filename) {
        std::string path = SDL_GetBasePath() + filename;
        auto it = path.rbegin();
        for (; *it != '_'; ++it);
        *it = '.';
        std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (file.fail()) {
            throw std::runtime_error("Impossibile caricare la risorsa " + filename);
        }
        reserve(file.tellg());
        file.seekg(std::ios::beg);
        assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    operator resource_view() const noexcept {
        return {data(), (int)size()};
    }
};

#define RESOURCE_NAME(name) __resource__##name
#define DECLARE_RESOURCE(name) static const resource RESOURCE_NAME(name) {#name};
#define GET_RESOURCE(name) RESOURCE_NAME(name)

#endif
#endif