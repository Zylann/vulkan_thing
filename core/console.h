#ifndef HEADER_CONSOLE_H
#define HEADER_CONSOLE_H

#include "types.h"
#include "string.h"

namespace Console {

void print_raw(const char *p_cstr);
void print_raw(const Char *p_cstr);

void print_line(const char *p_cstr);
void print_line(const Char *p_cstr);

void print_line(const String &s);

void print_line();

void pause();

} // namespace Console

#endif // HEADER_CONSOLE_H
