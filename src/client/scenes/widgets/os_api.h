#ifndef __OS_API_H__
#define __OS_API_H__

#ifdef WIN32

#include <Windows.h>

inline void play_bell() {
    MessageBeep(MB_ICONEXCLAMATION);
}

#else

inline void play_bell() {}

#endif

#endif