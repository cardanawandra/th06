// ----------------------------------------------------------------------------
//
// font.h - 文字表示
//
// Copyright (c) 2001 if (if@edokko.com)
// All Rights Reserved.
//
// ----------------------------------------------------------------------------
#pragma once

#include "sdl2_compat.hpp"

#define RELEASE(o)                                                                                                     \
    if (o)                                                                                                             \
    {                                                                                                                  \
        o = NULL;                                                                                                      \
    }

class CMyFont
{
  private:
    void *m_lpFont;

  public:
    CMyFont()
    {
        m_lpFont = NULL;
    };
    virtual void Init(void *lpDevice, int w, int h);
    virtual void Print(char *str, int x, int y, D3DCOLOR color = 0xffffffff);
    virtual void Clean();
};

  