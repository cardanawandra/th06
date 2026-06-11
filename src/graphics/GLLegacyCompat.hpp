#pragma once

#include <GL/gl.h>

// Texture env combine
#ifndef GL_COMBINE
#define GL_COMBINE                     0x8570
#endif

#ifndef GL_COMBINE_RGB
#define GL_COMBINE_RGB                 0x8571
#endif

#ifndef GL_COMBINE_ALPHA
#define GL_COMBINE_ALPHA               0x8572
#endif

#ifndef GL_SOURCE0_RGB
#define GL_SOURCE0_RGB                 0x8580
#endif

#ifndef GL_SOURCE1_RGB
#define GL_SOURCE1_RGB                 0x8581
#endif

#ifndef GL_SOURCE2_RGB
#define GL_SOURCE2_RGB                 0x8582
#endif

#ifndef GL_SOURCE0_ALPHA
#define GL_SOURCE0_ALPHA               0x8588
#endif

#ifndef GL_SOURCE1_ALPHA
#define GL_SOURCE1_ALPHA               0x8589
#endif

#ifndef GL_SOURCE2_ALPHA
#define GL_SOURCE2_ALPHA               0x858A
#endif

#ifndef GL_SRC0_RGB
#define GL_SRC0_RGB GL_SOURCE0_RGB
#endif

#ifndef GL_SRC1_RGB
#define GL_SRC1_RGB GL_SOURCE1_RGB
#endif

#ifndef GL_SRC2_RGB
#define GL_SRC2_RGB GL_SOURCE2_RGB
#endif

#ifndef GL_SRC0_ALPHA
#define GL_SRC0_ALPHA GL_SOURCE0_ALPHA
#endif

#ifndef GL_SRC1_ALPHA
#define GL_SRC1_ALPHA GL_SOURCE1_ALPHA
#endif

#ifndef GL_SRC2_ALPHA
#define GL_SRC2_ALPHA GL_SOURCE2_ALPHA
#endif

#ifndef GL_CONSTANT
#define GL_CONSTANT                    0x8576
#endif

#ifndef GL_PRIMARY_COLOR
#define GL_PRIMARY_COLOR               0x8577
#endif

// OpenGL 1.2 pixel formats/types
#ifndef GL_UNSIGNED_SHORT_5_5_5_1
#define GL_UNSIGNED_SHORT_5_5_5_1      0x8034
#endif

#ifndef GL_UNSIGNED_SHORT_4_4_4_4
#define GL_UNSIGNED_SHORT_4_4_4_4      0x8033
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#define GL_UNSIGNED_SHORT_5_6_5        0x8363
#endif

#ifndef GL_OPERAND0_ALPHA
#define GL_OPERAND0_ALPHA 0x8598
#endif

#ifndef GL_OPERAND1_ALPHA
#define GL_OPERAND1_ALPHA 0x8599
#endif

#ifndef GL_OPERAND2_ALPHA
#define GL_OPERAND2_ALPHA 0x859A
#endif

#ifndef GL_OPERAND0_RGB
#define GL_OPERAND0_RGB 0x8590
#endif

#ifndef GL_OPERAND1_RGB
#define GL_OPERAND1_RGB 0x8591
#endif

#ifndef GL_OPERAND2_RGB
#define GL_OPERAND2_RGB 0x8592
#endif