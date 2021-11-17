#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include <vector>
#include <fstream>
#include <iterator>

struct resource_view {
    const char *data;
    size_t length;
};

#define RESOURCE_NAME(name) __resource__##name
#define RESOURCE_LENGTH(name) __resource__##name##_length
#define DECLARE_RESOURCE(name) \
extern const char RESOURCE_NAME(name)[]; \
extern const unsigned long long int RESOURCE_LENGTH(name);
#define GET_RESOURCE(name) resource_view{RESOURCE_NAME(name), RESOURCE_LENGTH(name)}

struct resource : std::vector<char> {
    explicit resource(const std::string &filename) {
        std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (file.fail()) {
            throw std::runtime_error("Impossibile caricare la risorsa " + filename);
        }
        reserve(file.tellg());
        file.seekg(std::ios::beg);
        assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    operator resource_view() const noexcept {
        return {data(), size()};
    }
};

#endif