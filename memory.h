#ifndef HEADER_MEMORY_H
#define HEADER_MEMORY_H

#include <cstdlib>
#include <cstring> // memcpy etc

#define memalloc(nbytes) malloc(nbytes)
#define memrealloc(ptr, nbytes) realloc(ptr, nbytes)
#define memfree(ptr) free(ptr)

#endif // HEADER_MEMORY_H
