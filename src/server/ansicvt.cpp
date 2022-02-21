#ifdef WIN32
#include "ansicvt.h"

#include <windows.h>

std::string ansi_to_utf8(const std::string &src) {
    if (src.empty()) return std::string();
    int cp = GetACP();

    std::wstring dst(MultiByteToWideChar(cp, 0, src.data(), src.size(), nullptr, 0), L'\0');
    if (dst.empty() || !MultiByteToWideChar(cp, 0, src.data(), src.size(), dst.data(), dst.size())) return std::string();

    std::string ret(WideCharToMultiByte(CP_UTF8, 0, dst.data(), dst.size(), nullptr, 0, nullptr, nullptr), '\0');
    if (ret.empty() || !WideCharToMultiByte(CP_UTF8, 0, dst.data(), dst.size(), ret.data(), ret.size(), nullptr, nullptr)) return std::string();

    return ret;
}

#endif