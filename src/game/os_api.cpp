#include "os_api.h"

#ifdef WIN32

#include <Windows.h>

namespace os_api {

    void play_bell() {
        MessageBeep(MB_ICONEXCLAMATION);
    }

    void wait_for(int msecs) {
        Sleep(msecs);
    }

}

#else

#include <cstdio>
#include <unistd.h>

namespace os_api {

    void play_bell() {
        printf("\a");
    }

    void wait_for(int msecs) {
        usleep(msecs * 1000);
    }

}

#endif