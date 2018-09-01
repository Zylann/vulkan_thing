#ifndef HEADER_CONSOLE_H
#define HEADER_CONSOLE_H

#include "types.h"
#include "string.h"

namespace Console {

void print_raw(Char *p_cstr);
void print_line(Char *p_cstr);
void print_line(const String &s);

void pause();

} // namespace Console

#endif // HEADER_CONSOLE_H
