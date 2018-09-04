#include <cwchar>
#include <cstdio>
#include "console.h"

namespace Console {

void _print_raw(const char *p_cstr) {
    fputs(p_cstr, stdout);
}

void _print_raw(const Char *p_cstr) {
    fputws(p_cstr, stdout);
}

void _print_raw(const String &p_str) {
    fputws(p_str.data(), stdout);
}

void print_line() {
    fputwc(L'\n', stdout);
}

void pause() {
    getchar();
}

} // namespace Console
