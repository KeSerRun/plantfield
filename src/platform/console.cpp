#include "platform/console.h"

#ifdef _WIN32
#include <windows.h>
#endif

void setupConsoleUtf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}

void detachConsole() {
#ifdef _WIN32
    FreeConsole();
#endif
}
