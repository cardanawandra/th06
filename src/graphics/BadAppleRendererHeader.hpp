#include "BadAppleRenderer.hpp"
#include "../Supervisor.hpp"
#include "../GameWindow.hpp"
#include "../i18n.hpp"
#include <algorithm>
#include <stddef.h>
#include "../utils.hpp"
#include "../compat/Compat.hpp"
#include <math.h>

u8 ColorOpTableBadApple[3][4][4];

void InitColorOpTableBadApple()
{
    u32 factor;
    u32 value;

    for (factor = 0; factor < 4; ++factor)
    {
        for (value = 0; value < 4; ++value)
        {
            // MODULATE
            if (factor == 0 || value == 0)
            {
                ColorOpTableBadApple[COLOR_OP_MODULATE][factor][value] = 0;
            }
            else if (factor == 1 || value == 1)
            {
                ColorOpTableBadApple[COLOR_OP_MODULATE][factor][value] = 1;
            }
            else
            {
                ColorOpTableBadApple[COLOR_OP_MODULATE][factor][value] = 2;
            }

            // ADD
            if (factor == 0)
            {
                ColorOpTableBadApple[COLOR_OP_ADD][factor][value] = (u8)value;
            }
            else if (value == 0)
            {
                ColorOpTableBadApple[COLOR_OP_ADD][factor][value] = (u8)factor;
            }
            else if (factor == 2 || value == 2)
            {
                ColorOpTableBadApple[COLOR_OP_ADD][factor][value] = 2;
            }
            else
            {
                ColorOpTableBadApple[COLOR_OP_ADD][factor][value] = 1;
            }

            // REPLACE
            ColorOpTableBadApple[COLOR_OP_REPLACE][factor][value] = (u8)value;
        }
    }
}