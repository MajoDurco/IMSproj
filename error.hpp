
#ifndef error_hpp
#define error_hpp

#define DEBUG           1
#define LOG_WARNINGS    1

#include <iostream>

// Prints error message to stderr and exit with code
void Fatal(int code, const char *fmt, ...);

// Prints error message to stderr and exit with code
void Warning(const char *fmt, ...);

// Prints error message to stderr and exit with code
void Log(const char *fmt, ...);

#endif
