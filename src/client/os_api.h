#ifndef __OS_API_H__
#define __OS_API_H__

#include <filesystem>
#include <optional>

namespace os_api {

    void play_bell();

    std::optional<std::filesystem::path> open_file_dialog(
        const std::string &title,
        const std::filesystem::path &default_path,
        const std::string &filter,
        const std::string &description);

}

#endif