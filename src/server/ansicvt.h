#ifndef __ANSICVT_H__
#define __ANSICVT_H__

#include <string>

#ifdef WIN32

std::string ansi_to_utf8(const std::string &str);

#else

inline std::string ansi_to_utf8(const std::string &str) {
    return str;
}

#endif

#endif