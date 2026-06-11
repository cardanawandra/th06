cmake_minimum_required(VERSION 2.4)

project(th06_sdl)

add_definitions(

    -D_WINDOWS

    -DNO_SDL

    -D_CRT_SECURE_NO_DEPRECATE
)

# ---- Include + Lib dirs ----
include_directories(
    ${CMAKE_SOURCE_DIR}/../../src
)

# ---- Sources ----
set(TH06_GAME_SOURCES
    ../../src/main.cpp
    ../../src/AnmManager.cpp
    ../../src/AsciiManager.cpp
    ../../src/BombData.cpp
    ../../src/BulletData.cpp
    ../../src/BulletManager.cpp
    ../../src/Chain.cpp

    ../../src/controller/EmptyController.cpp

    ../../src/EclManager.cpp
    ../../src/EffectManager.cpp
    ../../src/Ending.cpp
    ../../src/EnemyEclInstr.cpp
    ../../src/EnemyManager.cpp
    ../../src/FileSystem.cpp
    ../../src/GameErrorContext.cpp
    ../../src/GameManager.cpp
    ../../src/GamePaths.cpp
    ../../src/GameWindow.cpp
    ../../src/Gui.cpp
    ../../src/ItemManager.cpp
    ../../src/MainMenu.cpp

    ../../src/sound/EmptyMidiOutput.cpp

    ../../src/MusicRoom.cpp
    ../../src/Player.cpp
    ../../src/ReplayManager.cpp
    ../../src/ResultScreen.cpp
    ../../src/Rng.cpp
    ../../src/ScreenEffect.cpp

    ../../src/sound/EmptySoundPlayer.cpp

    ../../src/Stage.cpp
    ../../src/Supervisor.cpp
    ../../src/TextHelper.cpp
    ../../src/utils.cpp
    ../../src/ZunMath.cpp
    ../../src/ZunTimer.cpp
    ../../src/thirdparty/sjis_converter.cpp
    ../../src/midi/MidiDefault.cpp

    ../../src/graphics/GLFunc.cpp
    ../../src/graphics/FixedFunctionGLWIN32.cpp

    ../../src/pbg3/FileAbstraction.cpp
    ../../src/pbg3/IPbg3Parser.cpp
    ../../src/pbg3/Pbg3Archive.cpp
    ../../src/pbg3/Pbg3Parser.cpp
)

# ---- Executable ----
add_executable(th06 ${TH06_GAME_SOURCES})

# ---- Link libraries (old style) ----
target_link_libraries(th06
    opengl32
    winmm
    
    ddraw
    dxguid
        
    gdi32
    user32
    kernel32
)

# ---- Custom command (simplified) ----
set(I18N_TPL "${CMAKE_SOURCE_DIR}/../../src/i18n.tpl")
set(I18N_HPP "${CMAKE_SOURCE_DIR}/../../src/i18n.hpp")

add_custom_command(
    OUTPUT ${I18N_HPP}
    COMMAND python ${CMAKE_SOURCE_DIR}/../../scripts/gen_i18n.py
            ${I18N_TPL} ${I18N_HPP} utf-8
    DEPENDS ${I18N_TPL}
)

add_custom_target(generate_i18n ALL DEPENDS ${I18N_HPP})
add_dependencies(th06 generate_i18n)