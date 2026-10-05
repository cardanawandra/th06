cmake_minimum_required(VERSION 3.6)

project(th06 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# i18n.hpp generation
# ============================================================================

get_filename_component(TH06_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.."
    ABSOLUTE
)

# ============================================================================
# Game sources
# ============================================================================

set(TH06_GAME_SOURCES
    ${TH06_ROOT}/src/main.cpp
    ${TH06_ROOT}/src/AnmManager.cpp
    ${TH06_ROOT}/src/AsciiManager.cpp
    ${TH06_ROOT}/src/BombData.cpp
    ${TH06_ROOT}/src/BulletData.cpp
    ${TH06_ROOT}/src/BulletManager.cpp
    ${TH06_ROOT}/src/Chain.cpp

    ${TH06_ROOT}/src/controller/SDLController.cpp

    ${TH06_ROOT}/src/EclManager.cpp
    ${TH06_ROOT}/src/EffectManager.cpp
    ${TH06_ROOT}/src/Ending.cpp
    ${TH06_ROOT}/src/EnemyEclInstr.cpp
    ${TH06_ROOT}/src/EnemyManager.cpp
    ${TH06_ROOT}/src/FileSystem.cpp
    ${TH06_ROOT}/src/GameErrorContext.cpp
    ${TH06_ROOT}/src/GameManager.cpp
    ${TH06_ROOT}/src/GamePaths.cpp
    ${TH06_ROOT}/src/GameWindow.cpp
    ${TH06_ROOT}/src/Gui.cpp
    ${TH06_ROOT}/src/ItemManager.cpp
    ${TH06_ROOT}/src/MainMenu.cpp

    ${TH06_ROOT}/src/sound/SDLMidiOutput.cpp

    ${TH06_ROOT}/src/MusicRoom.cpp
    ${TH06_ROOT}/src/Player.cpp
    ${TH06_ROOT}/src/ReplayManager.cpp
    ${TH06_ROOT}/src/ResultScreen.cpp
    ${TH06_ROOT}/src/Rng.cpp
    ${TH06_ROOT}/src/ScreenEffect.cpp

    ${TH06_ROOT}/src/sound/SDLSoundPlayer.cpp

    ${TH06_ROOT}/src/Stage.cpp
    ${TH06_ROOT}/src/Supervisor.cpp
    ${TH06_ROOT}/src/TextHelper.cpp
    ${TH06_ROOT}/src/utils.cpp
    ${TH06_ROOT}/src/ZunMath.cpp
    ${TH06_ROOT}/src/ZunTimer.cpp
    # pbg3 archive
    ${TH06_ROOT}/src/pbg3/FileAbstraction.cpp
    ${TH06_ROOT}/src/pbg3/IPbg3Parser.cpp
    ${TH06_ROOT}/src/pbg3/Pbg3Archive.cpp
    ${TH06_ROOT}/src/pbg3/Pbg3Parser.cpp
    # graphics
    ${TH06_ROOT}/src/thirdparty/sjis_converter.cpp
    ${TH06_ROOT}/src/midi/MidiDefault.cpp
    ${TH06_ROOT}/src/graphics/Software2D.cpp
    ${TH06_ROOT}/src/graphics/GLFunc.cpp
    ${TH06_ROOT}/src/graphics/WebGL.cpp
)

# ============================================================================
# Game shared library
# ============================================================================

add_library(main SHARED
    ${TH06_GAME_SOURCES}
)

# ============================================================================
# Android / portable definitions
# ============================================================================

target_compile_definitions(main PRIVATE
#    NO_LOG_COMPAT=1

    TH06_ANDROID=1
    NO_FIXED_FUNCTION=1
    COMPAT_PORTABLE=1
    SDL_ENABLE_OLD_NAMES=1

#    NO_WEBGL=1
#    NO_EFFECT=1
)

# ============================================================================
# Compiler options
# ============================================================================

target_compile_options(main PRIVATE
    -fpermissive
    -Wno-extra-qualification
    -Wno-unknown-warning-option
    -Wno-unknown-pragmas

    -Wno-unused-value
    -Wno-macro-redefined
    -Wno-null-conversion
    -Wno-inconsistent-missing-override
    -Wno-switch
    -Wno-null-arithmetic
    -Wno-constant-conversion

    -Wno-address-of-temporary
    -Wno-format-security
)

# ============================================================================
# Include directories
# ============================================================================

target_include_directories(main PRIVATE
    "${TH06_ROOT}/src"
    "../../../SDL3/include/SDL3"
)

# ============================================================================
# i18n dependency
# ============================================================================

if(TARGET generate_i18n)
    add_dependencies(main generate_i18n)
endif()

# ============================================================================
# SDL3
# ============================================================================

target_link_libraries(main PRIVATE
    SDL3::SDL3
)

# ============================================================================
# Android system libraries
# ============================================================================

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles2-lib GLESv2)

target_link_libraries(main PRIVATE
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles2-lib}
)