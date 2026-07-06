#include "Software.hpp"
#include "../Supervisor.hpp"
#include "../GameWindow.hpp"
#include "../i18n.hpp"
#include <algorithm>
#include <stddef.h>
#include "../utils.hpp"
#include "../compat/Compat.hpp"
#include <math.h>

u8 alphaThreshold = 4;

u8 ColorOpTable[3][256][256];
u8 ColorDA[2][256];
void InitColorOpTable()
{
    i16 i;
    for (i=0;i < 256; i++){
        ColorDA[BLEND_INV_SRC_ALPHA][i] = 255-i;
        ColorDA[BLEND_ONE][i] = 255;
    }
    u16 factor, value;
    for (factor = 0; factor < 256; ++factor)
    {
        for (value = 0; value < 256; ++value)
        {
            // Modulate
            ColorOpTable[COLOR_OP_MODULATE][factor][value] =
                u8((value * factor) >> 8);

            // Add
            if(factor+value>255){
                ColorOpTable[COLOR_OP_ADD][factor][value] = 255;
            }else{
                ColorOpTable[COLOR_OP_ADD][factor][value] = factor+value;
            }

            // replace (bruh)
            ColorOpTable[COLOR_OP_REPLACE][factor][value] = value;
        }
    }
}