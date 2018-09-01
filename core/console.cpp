#include <cwchar>
#include <cstdio>
#include "console.h"

namespace Console {

void print_raw(Char *p_cstr) {
    fputws(p_cstr, stdout);
}

void print_line(Char *p_cstr) {
    fputws(p_cstr, stdout);
    fputwc(L'\n', stdout);
}

void print_line(const String &s) {
    print_line(s.data());
}

void pause() {
    getchar();
}

} // namespace Console
