#pragma once

#include <stdlib.h>

namespace ZunMemory
{
inline void *Alloc(size_t size)
{
    return malloc(size);
}

inline void Free(void *ptr)
{
    free(ptr);
}
}; // namespace ZunMemory
