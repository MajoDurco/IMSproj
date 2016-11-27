
#ifndef error_hpp
#define error_hpp

// Define DEBUG macro if it is not already defined
#ifndef DEBUG
    #define DEBUG           1
#endif
#define LOG_WARNINGS    1

#include <iostream>

/** Prints error message to stderr and exit with code */
void Fatal(int code, const char *fmt, ...);

/** Prints error message to stderr */
void Warning(const char *fmt, ...);

/** Prints message to stdout */
void Log(const char *fmt, ...);

#endif
