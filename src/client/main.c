#include <windows.h>
#include <string.h>

__declspec(dllimport) long __stdcall entrypoint(const char *base_path);

int main(int argc, char **argv) {
    char base_path[256];
    strcpy(base_path, argv[0]);

    char *last_slash = NULL;
    for (char *c = base_path; *c != '\0'; ++c) {
        if (*c == '\\' || *c == '/') {
            last_slash = c;
        }
    }
    *(last_slash + 1) = '\0';
    return entrypoint(base_path);
}