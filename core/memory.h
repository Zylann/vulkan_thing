#ifndef HEADER_MEMORY_H
#define HEADER_MEMORY_H

#include <cstdlib>
#include <cstring> // memcpy etc

namespace Memory {

void *alloc(size_t nbytes, const char *file, int line);
void *realloc(void *ptr, size_t nbytes, const char *file, int line);
void free(void *ptr, const char *file, int line);

size_t get_alloc_count();

} // namespace Memory

#define memalloc(nbytes) Memory::alloc(nbytes, __FILE__, __LINE__)
#define memrealloc(ptr, nbytes) Memory::realloc(ptr, nbytes, __FILE__, __LINE__)
#define memfree(ptr) Memory::free(ptr, __FILE__, __LINE__)

#endif // HEADER_MEMORY_H
