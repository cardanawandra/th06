#include "BadAppleRenderer.hpp"
#include "../Supervisor.hpp"
#include "../GameWindow.hpp"
#include "../i18n.hpp"
#include <algorithm>
#include <stddef.h>
#include "../utils.hpp"
#include "../compat/Compat.hpp"
#include <math.h>

BAD_APPLE_FRAMEBUFFER_TYPE ColorOpTableBadApple[3][4][4];

void InitColorOpTableBadApple()
{
    for (u32 factor = 0; factor < 4; ++factor)
    {
        for (u32 value = 0; value < 4; ++value)
        {
            // MODULATE
            if (factor == 0 || value == 0)
            {
                ColorOpTableBadApple[COLOR_OP_MODULATE][factor][value] =
                    BAD_APPLE_COLOR_TRANSPARENT;
            }
            else if (factor == 1 || value == 1)
            {
                ColorOpTableBadApple[COLOR_OP_MODULATE][factor][value] =
                    BAD_APPLE_COLOR_BLACK;
            }
            else
            {
                ColorOpTableBadApple[COLOR_OP_MODULATE][factor][value] =
                    BAD_APPLE_COLOR_WHITE;
            }

            // ADD
            if (factor == 0)
            {
                ColorOpTableBadApple[COLOR_OP_ADD][factor][value] =
                    value == 0 ? BAD_APPLE_COLOR_TRANSPARENT :
                    value == 1 ? BAD_APPLE_COLOR_BLACK :
                                 BAD_APPLE_COLOR_WHITE;
            }
            else if (value == 0)
            {
                ColorOpTableBadApple[COLOR_OP_ADD][factor][value] =
                    factor == 0 ? BAD_APPLE_COLOR_TRANSPARENT :
                    factor == 1 ? BAD_APPLE_COLOR_BLACK :
                                  BAD_APPLE_COLOR_WHITE;
            }
            else if (factor == 2 || value == 2)
            {
                ColorOpTableBadApple[COLOR_OP_ADD][factor][value] =
                    BAD_APPLE_COLOR_WHITE;
            }
            else
            {
                ColorOpTableBadApple[COLOR_OP_ADD][factor][value] =
                    BAD_APPLE_COLOR_BLACK;
            }

            // REPLACE
            ColorOpTableBadApple[COLOR_OP_REPLACE][factor][value] =
                value == 0 ? BAD_APPLE_COLOR_TRANSPARENT :
                value == 1 ? BAD_APPLE_COLOR_BLACK :
                             BAD_APPLE_COLOR_WHITE;
        }
    }
}