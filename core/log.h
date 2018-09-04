#ifndef HEADER_LOG_H
#define HEADER_LOG_H

#include "console.h"

namespace Log {

extern const Char *DEBUG_PREFIX;
extern const Char *INFO_PREFIX;
extern const Char *WARNING_PREFIX;
extern const Char *ERROR_PREFIX;

template <typename ...Args>
inline void debug(const Args&... args) {
    Console::_print_raw(DEBUG_PREFIX);
    Console::print_line(args...);
}

template <typename ...Args>
inline void info(const Args&... args) {
    Console::_print_raw(INFO_PREFIX);
    Console::print_line(args...);
}

template <typename ...Args>
inline void warning(const Args&... args) {
    Console::_print_raw(WARNING_PREFIX);
    Console::print_line(args...);
}

template <typename ...Args>
inline void error(const Args&... args) {
    Console::_print_raw(ERROR_PREFIX);
    Console::print_line(args...);
}

} // namespace Log

#endif // HEADER_LOG_H
