#include "os_api.h"

#ifdef WIN32
#include <Windows.h>
#include <commdlg.h>

#include <memory>
#include <cstring>

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
    const std::string &description)
{
    OPENFILENAMEW ofn{sizeof(OPENFILENAMEW)};

    std::wstring wfilter = fmt::format(L"{0} ({1})\0{1}\0",
        utf8_to_wstring(description), utf8_to_wstring(filter));
    ofn.lpstrFilter = wfilter.c_str();

    auto file_buf = std::make_unique<wchar_t[]>(MAX_PATH);
    std::wstring wfile = default_path.wstring();
    std::memcpy(file_buf.get(), wfile.c_str(), wfile.size() * sizeof(wchar_t));
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
    const std::string &description)
{
    return std::nullopt;
}

}

#endif