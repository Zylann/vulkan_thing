#ifndef HEADER_CONSOLE_H
#define HEADER_CONSOLE_H

#include "types.h"
#include "string.h"

namespace Console {

void _print_raw(const char *p_cstr);
void _print_raw(const Char *p_cstr);
void print_line();

template <typename T>
inline void print_raw(const T a) {
    String s;
    to_string(s, a);
    _print_raw(s.data());
}

template <>
inline void print_raw(const char *p_cstr) {
    _print_raw(p_cstr);
}
template <>
inline void print_raw(const Char *p_cstr) {
    _print_raw(p_cstr);
}
template <>
inline void print_raw(const String &p_str) {
    _print_raw(p_str.data());
}

template <typename T>
inline void print_line(const T a) {
    print_raw(a);
    print_line();
}

template <typename T, typename ...Args>
inline void print_line(const T &a, const Args&... args) {
    print_raw(a);
    print_line(args...);
}

void pause();

} // namespace Console

#endif // HEADER_CONSOLE_H
