#include <string.h>

#include "bangclient_export.h"

#ifdef WIN32
    #include <windows.h>
    #define STDCALL __stdcall
#else
    #define STDCALL
#endif

BANGCLIENT_EXPORT long STDCALL entrypoint(const char *base_path);

#define BUFFER_SIZE 256

int main(int argc, char **argv) {
    char base_path[BUFFER_SIZE];
    strcpy_s(base_path, BUFFER_SIZE, argv[0]);

    char *last_slash = NULL;
    for (char *c = base_path; *c != '\0'; ++c) {
        if (*c == '\\' || *c == '/') {
            last_slash = c;
        }
    }
    *(last_slash + 1) = '\0';
    return entrypoint(base_path);
}