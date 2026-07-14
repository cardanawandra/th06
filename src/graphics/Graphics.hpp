#ifdef WIN32
// #include "graphics/FixedFunctionDX2.hpp"
#endif

#include "graphics/Software.hpp"
#ifdef NO_SDL
    // #include "graphics/FixedFunctionGLWIN32.hpp"
    #ifdef USE_SFML
        #include "graphics/FixedFunctionGLSFML.hpp"
    #endif
#else
    #include <SDL.h>

    #ifndef NO_FIXED_FUNCTION
    #include "graphics/FixedFunctionGL.hpp"
    #endif
    #if SDL_MAJOR_VERSION >= 2
        #include "graphics/WebGL.hpp"
        #ifndef COMPAT_PORTABLE
        #endif
    #endif
    #if SDL_MAJOR_VERSION >= 3
        #include "graphics/Hardware.hpp"
    #endif
#endif