#ifndef HEADER_TYPES_H
#define HEADER_TYPES_H

#include <cstddef>

typedef wchar_t Char;

#ifdef _MSC_VER

typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;

#else

#ifdef NO_STDINT_H
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
#else
#include <stdint.h>
#endif

#endif

#endif // HEADER_TYPES_H
