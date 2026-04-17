//-----------------------------------------------------------------------------
// File: DXUtil.h - SDL2 version (no COM Release)
//-----------------------------------------------------------------------------
#ifndef DXUTIL_H
#define DXUTIL_H

#define SAFE_DELETE(p)                                                                                                 \
    {                                                                                                                  \
        if (p)                                                                                                         \
        {                                                                                                              \
            delete (p);                                                                                                \
            (p) = NULL;                                                                                                \
        }                                                                                                              \
    }
#define SAFE_DELETE_ARRAY(p)                                                                                           \
    {                                                                                                                  \
        if (p)                                                                                                         \
        {                                                                                                              \
            delete[] (p);                                                                                              \
            (p) = NULL;                                                                                                \
        }                                                                                                              \
    }
#define SAFE_RELEASE(p)                                                                                                \
    {                                                                                                                  \
        if (p)                                                                                                         \
        {                                                                                                              \
            (p) = NULL;                                                                                                \
        }                                                                                                              \
    }

#endif // DXUTIL_H
