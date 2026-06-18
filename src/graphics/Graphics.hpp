#ifdef WIN32
// #include "graphics/FixedFunctionDX2.hpp"
#endif

#ifdef NO_SDL
    #include "graphics/Software.hpp"
    // #include "graphics/FixedFunctionGLWIN32.hpp"
    #ifdef USE_SFML
        #include "graphics/FixedFunctionGLSFML.hpp"
    #endif
#else
    #include <SDL.h>
    
    #include "graphics/Software.hpp"
    #if SDL_MAJOR_VERSION >= 2
        #ifdef __ANDROID__
            #include "graphics/WebGL.hpp"
        #else
            #include "graphics/FixedFunctionGL.hpp"
        #endif
    #endif
    #if SDL_MAJOR_VERSION >= 3
        #include "graphics/Hardware.hpp"
    #endif
#endif