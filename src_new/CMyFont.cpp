// ----------------------------------------------------------------------------
//
// font.cpp - フォント描画部分
//
// Copyright (c) 2001 if (if@edokko.com)
// All Rights Reserved.
//
// ----------------------------------------------------------------------------

#include "CMyFont.hpp"
#include "GameWindow.hpp"
#include "i18n.hpp"

void CMyFont::Init(void *lpDevice, int w, int h)
{
    (void)lpDevice;
    (void)w;
    (void)h;
}
// ----------------------------------------------------------------------------
void CMyFont::Print(char *str, int x, int y, D3DCOLOR color)
{
    (void)str;
    (void)x;
    (void)y;
    (void)color;
}
// ----------------------------------------------------------------------------
void CMyFont::Clean()
{
    RELEASE(m_lpFont);
}
  