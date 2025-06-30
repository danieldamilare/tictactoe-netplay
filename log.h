#ifndef LOG_H
#define LOG_H

#include <iostream>
#ifdef LOGSOCK
#include <format>

#define LOG(fmt, ...) do { \
    std::cerr << std::format(fmt, ##__VA_ARGS__) << '\n'; \
} while(0)

#else
#define LOG(fmt, ...) ((void)0)
#endif

#endif

