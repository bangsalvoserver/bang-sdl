#include "os_api.h"

#include <SDL2/SDL_syswm.h>

#ifdef WIN32
#include <Windows.h>
#include <commdlg.h>

#include <memory>
#include <cwchar>

#include <fmt/xchar.h>

namespace os_api {

void play_bell() {
    MessageBeep(MB_ICONEXCLAMATION);
}

inline std::wstring utf8_to_wstring(const std::string &str) {
    std::wstring dst(MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), nullptr, 0), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), dst.data(), dst.size());
    return dst;
}

std::optional<std::filesystem::path> open_file_dialog(
    const std::string &title,
    const std::filesystem::path &default_path,
    const std::string &filter,
    const std::string &description,
    sdl::window *parent)
{
    OPENFILENAMEW ofn{sizeof(OPENFILENAMEW)};

    if (parent) {
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        SDL_GetWindowWMInfo(parent->get(), &wminfo);
        ofn.hwndOwner = wminfo.info.win.window;
    }

    using namespace std::string_view_literals;
    std::wstring wfilter = utf8_to_wstring(fmt::format("{0} ({1})\0{1}\0"sv, description, filter));
    ofn.lpstrFilter = wfilter.c_str();

    auto file_buf = std::make_unique_for_overwrite<wchar_t[]>(MAX_PATH);
    std::wcsncpy(file_buf.get(), default_path.wstring().c_str(), MAX_PATH);
    ofn.lpstrFile = file_buf.get();
    ofn.nMaxFile = MAX_PATH;

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

namespace os_api {

void play_bell() {
    printf("\a");
}

std::optional<std::filesystem::path> open_file_dialog(
    const std::string &title,
    const std::filesystem::path &default_path,
    const std::string &filter,
    const std::string &description,
    sdl::window *parent)
{
    return std::nullopt;
}

}

#endif