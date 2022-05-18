#ifndef __OS_API_H__
#define __OS_API_H__

#include <filesystem>
#include <optional>

#include "sdl_wrap.h"

namespace os_api {

    void play_bell();

    struct file_filter {
        std::initializer_list<std::string> filters;
        std::string description;
    };

    std::optional<std::filesystem::path> open_file_dialog(
        const std::string &title,
        const std::filesystem::path &default_path,
        const std::initializer_list<file_filter> &filters,
        sdl::window *parent = nullptr);

}

#endif