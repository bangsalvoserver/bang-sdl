#include "os_api.h"

#ifdef WIN32

#include <Windows.h>

namespace widgets {

    void play_bell() {
        MessageBeep(MB_ICONEXCLAMATION);
    }

}

#else

namespace widgets {

    void play_bell() {}

}

#endif