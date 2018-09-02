#include "memory.h"
//#include <cstdio> // For debug printf
#include <atomic>

namespace Memory {

std::atomic<size_t> g_alloc_count = 0;

void *alloc(size_t nbytes, const char *file, int line) {

    //printf("Alloc at %s: %i\n", file, line);
    ++g_alloc_count;

    return ::malloc(nbytes);
}

void *realloc(void *ptr, size_t nbytes, const char *file, int line) {

    //printf("Realloc at %s: %i\n", file, line);

    // TODO How do I get size of that damn pointer
    return ::realloc(ptr, nbytes);
}

void free(void *ptr, const char *file, int line) {

    //printf("Free at %s: %i\n", file, line);
    --g_alloc_count;

    ::free(ptr);
}

size_t get_alloc_count() {
    return g_alloc_count;
}

} // namespace Memory


