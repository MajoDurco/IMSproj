
#include "error.hpp"

/** Prints error message to stderr and exit with code */
void Fatal(int code, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    if (code) {
        fprintf(stderr, "[FATAL]: (%i): ", code);
    }
    vfprintf(stderr, fmt, list);
    printf("\n");
    va_end(list);
    exit(code);
}

/** Prints error message to stderr */
void Warning(const char *fmt, ...) {
#if LOG_WARNINGS
    va_list list;
    va_start(list, fmt);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, fmt, list);
    printf("\n");
    va_end(list);
#endif
}

/** Prints message to stdout */
void Log(const char *fmt, ...) {
#if DEBUG
    va_list list;
    va_start(list, fmt);
    fprintf(stderr, "LOG: ");
    vfprintf(stderr, fmt, list);
    printf("\n");
    va_end(list);
#endif
}
