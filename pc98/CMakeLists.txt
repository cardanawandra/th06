cmake_minimum_required(VERSION 2.4)

project(th06_sdl)

option(TH06_USE_SDL "TH06 SDL" ON)

# ---- Paths ----
set(SDL_DIR "${CMAKE_SOURCE_DIR}/../3rdparty/SDL")
set(SDL_IMAGE_DIR "${CMAKE_SOURCE_DIR}/../3rdparty/SDL_image")
set(SDL_MIXER_DIR "${CMAKE_SOURCE_DIR}/../3rdparty/SDL_mixer")
set(SDL_TTF_DIR "${CMAKE_SOURCE_DIR}/../3rdparty/SDL_ttf")

# No 64-bit support in VC6 realistically → force x86
set(SDL_ARCH "x86")

# ---- Include + Lib dirs ----
include_directories(
    ${SDL_DIR}/include
    ${SDL_IMAGE_DIR}/include
    ${SDL_TTF_DIR}/include
    ${CMAKE_SOURCE_DIR}/../src
)

link_directories(
    ${SDL_DIR}/lib/${SDL_ARCH}
    ${SDL_IMAGE_DIR}/lib
    ${SDL_TTF_DIR}/lib
)

# ---- Sources ----
set(TH06_GAME_SOURCES
    ../src/main.cpp
    ../src/GLFunc.cpp
    ../src/AnmManager.cpp
    ../src/AsciiManager.cpp
    ../src/BombData.cpp
    ../src/BulletData.cpp
    ../src/BulletManager.cpp
    ../src/Chain.cpp
    ../src/Controller.cpp
    ../src/EclManager.cpp
    ../src/EffectManager.cpp
    ../src/Ending.cpp
    ../src/EnemyEclInstr.cpp
    ../src/EnemyManager.cpp
    ../src/FileSystem.cpp
    ../src/GameErrorContext.cpp
    ../src/GameManager.cpp
    ../src/GamePaths.cpp
    ../src/GameWindow.cpp
    ../src/Gui.cpp
    ../src/ItemManager.cpp
    ../src/MainMenu.cpp
    ../src/MidiOutput.cpp
    ../src/MusicRoom.cpp
    ../src/Player.cpp
    ../src/ReplayManager.cpp
    ../src/ResultScreen.cpp
    ../src/Rng.cpp
    ../src/ScreenEffect.cpp
    ../src/SoundPlayer.cpp
    ../src/Stage.cpp
    ../src/Supervisor.cpp
    ../src/TextHelper.cpp
    ../src/utils.cpp
    ../src/ZunTimer.cpp
    ../src/midi/MidiDefault.cpp
    ../src/graphics/FixedFunctionGL.cpp
    ../src/graphics/WebGL.cpp
    ../src/pbg3/FileAbstraction.cpp
    ../src/pbg3/IPbg3Parser.cpp
    ../src/pbg3/Pbg3Archive.cpp
    ../src/pbg3/Pbg3Parser.cpp
)

# ---- Executable ----
add_executable(th06 ${TH06_GAME_SOURCES})

# ---- Link libraries (old style) ----
target_link_libraries(th06
    SDLmain
    SDL
    SDL_image
    SDL_ttf
    opengl32
    winmm
)

# ---- Custom command (simplified) ----
set(I18N_TPL "${CMAKE_SOURCE_DIR}/../src/i18n.tpl")
set(I18N_HPP "${CMAKE_SOURCE_DIR}/../src/i18n.hpp")

add_custom_command(
    OUTPUT ${I18N_HPP}
    COMMAND python ${CMAKE_SOURCE_DIR}/../scripts/gen_i18n.py
            ${I18N_TPL} ${I18N_HPP} utf-8
    DEPENDS ${I18N_TPL}
)

add_custom_target(generate_i18n ALL DEPENDS ${I18N_HPP})
add_dependencies(th06 generate_i18n)