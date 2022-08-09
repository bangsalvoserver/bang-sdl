#include "os_api.h"

#include <SDL2/SDL_syswm.h>

static std::string string_join(const std::initializer_list<std::string> &strs, std::string_view delim) {
    std::string ret;
    auto it = strs.begin();
    if (it != strs.end()) {
        ret = *it;
    }
    for (++it; it != strs.end(); ++it) {
        ret.append(delim);
        ret.append(*it);
    }
    return ret;
}

#ifdef WIN32
#include <Windows.h>
#include <commdlg.h>

#include <memory>
#include <cwchar>
#include <array>

#include <fmt/xchar.h>

namespace os_api {

void play_bell() {
    MessageBeep(MB_ICONEXCLAMATION);
}

inline std::wstring utf8_to_wstring(const std::string &str) {
    std::wstring dst(MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), dst.data(), static_cast<int>(dst.size()));
    return dst;
}

std::optional<std::filesystem::path> open_file_dialog(
    const std::string &title,
    const std::filesystem::path &default_path,
    const std::initializer_list<file_filter> &filters,
    sdl::window *parent)
{
    OPENFILENAMEW ofn{sizeof(OPENFILENAMEW)};

    if (parent) {
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        SDL_GetWindowWMInfo(parent->get(), &wminfo);
        ofn.hwndOwner = wminfo.info.win.window;
    }

    std::string filter;
    for (const auto &[fs, description] : filters) {
        using namespace std::string_view_literals;
        const std::string filter_str = string_join(fs, ";");
        if (description.empty()) {
            filter.append(fmt::format("{0}\0{0}\0"sv, filter_str));
        } else {
            filter.append(fmt::format("{0} ({1})\0{1}\0"sv, description, filter_str));
        }
    }
    std::wstring wfilter = utf8_to_wstring(filter);
    ofn.lpstrFilter = wfilter.c_str();

    std::array<wchar_t, MAX_PATH> file_buf;
    std::wcsncpy(file_buf.data(), default_path.wstring().c_str(), file_buf.size());
    ofn.lpstrFile = file_buf.data();
    ofn.nMaxFile = static_cast<int>(file_buf.size());

    std::wstring wtitle = utf8_to_wstring(title);
    ofn.lpstrTitle = wtitle.c_str();

    if (!GetOpenFileNameW(&ofn)) {
        return std::nullopt;
    }

    return std::filesystem::path(ofn.lpstrFile);
}

}

#else
#include <cstdio>
#include <iomanip>
#include <ranges>

namespace os_api {

void play_bell() {
    printf("\a");
}

std::optional<std::filesystem::path> open_file_dialog(
    const std::string &title,
    const std::filesystem::path &default_path,
    const std::initializer_list<file_filter> &filters,
    sdl::window *parent)
{
    std::stringstream zenity_command;
    zenity_command << "zenity --file-selection";
    if (!title.empty()) {
        zenity_command << " --title=" << std::quoted(title);
    }
    if (!default_path.empty()) {
        zenity_command << " --filename=" << std::quoted(default_path.string());
    }
    for (const auto &[fs, description] : filters) {
        std::string str = string_join(fs, " ");
        if (!description.empty()) {
            str = fmt::format("{0} ({1}) | {1}", description, str);
        }
        zenity_command << " --file-filter=" << std::quoted(str);
    }

    std::array<char, 1024> file_buf;

    FILE *f = popen(zenity_command.str().c_str(), "r");
    fgets(file_buf.data(), file_buf.size(), f);
    if (fclose(f) == 0) {
        return std::filesystem::path(std::string_view(file_buf.data(), strlen(file_buf.data()) - 1));
    }
    return std::nullopt;
}

}

#endif