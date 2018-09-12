#ifndef HEADER_MACROS_H
#define HEADER_MACROS_H

#include "log.h"

#define ERR_FAIL_COND_V(cond, v) \
if(cond) { \
    Log::error(__FILE__, ": ", __LINE__, ": `", #cond, "` is false"); \
    return v; \
}

#define CHECK_RESULT_V(f, v) \
{ \
    VkResult result = f; \
    if (result != VK_SUCCESS) { \
        Log::error(__FILE__, ": ", __LINE__, ": `", #f, "`: failed with result ", result); \
        return v; \
    } \
}

#endif // HEADER_MACROS_H
