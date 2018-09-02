#include <cwchar>
#include <cstdio>
#include "console.h"

namespace Console {

void print_raw(const char *p_cstr) {
    fputs(p_cstr, stdout);
}

void print_raw(const Char *p_cstr) {
    fputws(p_cstr, stdout);
}

void print_line(const char *p_cstr) {
    fputs(p_cstr, stdout);
    fputc('\n', stdout);
}

void print_line(const Char *p_cstr) {
    fputws(p_cstr, stdout);
    fputwc(L'\n', stdout);
}

void print_line(const String &s) {
    print_line(s.data());
}

void print_line() {
    fputwc(L'\n', stdout);
}

void pause() {
    getchar();
}

} // namespace Console
