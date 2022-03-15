#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#include <vector>
#include <fstream>
#include <iterator>
#include <filesystem>
#include <fmt/core.h>

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
    using std::vector<char>::vector;
    
    explicit resource(const std::filesystem::path &filename) {
        std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
        if (file.fail()) {
            throw std::runtime_error(fmt::format("Could not open {}", filename.string()));
        }
        assign(file.tellg(), '\0');
        file.seekg(std::ios::beg);
        file.read(data(), size());
    }

    resource() = default;

    operator resource_view() const {
        return {data(), size()};
    }

    operator std::string_view() const {
        return {data(), size()};
    }
};

#endif