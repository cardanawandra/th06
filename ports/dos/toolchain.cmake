set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR i386)

# compilers from build-djgpp
set(CMAKE_C_COMPILER i586-pc-msdosdjgpp-gcc)
set(CMAKE_CXX_COMPILER i586-pc-msdosdjgpp-g++)

set(CMAKE_AR i586-pc-msdosdjgpp-ar)
set(CMAKE_RANLIB i586-pc-msdosdjgpp-ranlib)

# important for cross compile
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_EXECUTABLE_SUFFIX ".exe")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)