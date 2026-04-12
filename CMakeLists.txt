cmake_minimum_required(VERSION 3.20)
project(th06_sdl2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Option: build with OpenGL ES 2.0 shader-based renderer instead of fixed-function GL
option(USE_GLES "Build with OpenGL ES 2.0 renderer (RendererGLES) instead of fixed-function RendererGL" OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TH06_USE_SDL2 "TH06 SDL" ON)
option(TH06_BUILD_ORIGINAL "D3D8" OFF)

if(TH06_USE_SDL2)
  if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_ARCH "x64")
    else()
        set(SDL2_ARCH "x86")
    endif()

    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2")
    set(SDL2_IMAGE_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2_image")
    set(SDL2_MIXER_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2_mixer")
    set(SDL2_TTF_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2_ttf")

    add_library(SDL2::SDL2 SHARED IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.dll"
        IMPORTED_IMPLIB   "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_DIR}/include"
    )

    add_library(SDL2::SDL2main STATIC IMPORTED)
    set_target_properties(SDL2::SDL2main PROPERTIES
        IMPORTED_LOCATION "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2main.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_DIR}/include"
    )

    add_library(SDL2_image::SDL2_image SHARED IMPORTED)
    set_target_properties(SDL2_image::SDL2_image PROPERTIES
        IMPORTED_LOCATION "${SDL2_IMAGE_DIR}/lib/${SDL2_ARCH}/SDL2_image.dll"
        IMPORTED_IMPLIB   "${SDL2_IMAGE_DIR}/lib/${SDL2_ARCH}/SDL2_image.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_DIR}/include"
    )

    add_library(SDL2_mixer::SDL2_mixer SHARED IMPORTED)
    set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
        IMPORTED_LOCATION "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.dll"
        IMPORTED_IMPLIB   "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_MIXER_DIR}/include"
    )

    add_library(SDL2_ttf::SDL2_ttf SHARED IMPORTED)
    set_target_properties(SDL2_ttf::SDL2_ttf PROPERTIES
        IMPORTED_LOCATION "${SDL2_TTF_DIR}/lib/${SDL2_ARCH}/SDL2_ttf.dll"
        IMPORTED_IMPLIB   "${SDL2_TTF_DIR}/lib/${SDL2_ARCH}/SDL2_ttf.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_DIR}/include"
    )

    message(STATUS "SDL2 DIR: ${SDL2_DIR}")
    message(STATUS "SDL2_image DIR: ${SDL2_IMAGE_DIR}")
    message(STATUS "SDL2_mixer DIR: ${SDL2_MIXER_DIR}")
    message(STATUS "SDL2_ttf DIR: ${SDL2_TTF_DIR}")
    message(STATUS "目标架构: ${SDL2_ARCH}")
  else()
    # Linux/macOS: 通过 pkg-config 查找系统安装的 SDL2
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
    pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)

    # 创建兼容的 target 别名
    if(NOT TARGET SDL2::SDL2)
        add_library(SDL2::SDL2 ALIAS PkgConfig::SDL2)
    endif()
    if(NOT TARGET SDL2_image::SDL2_image)
        add_library(SDL2_image::SDL2_image ALIAS PkgConfig::SDL2_IMAGE)
    endif()
    if(NOT TARGET SDL2_mixer::SDL2_mixer)
        add_library(SDL2_mixer::SDL2_mixer ALIAS PkgConfig::SDL2_MIXER)
    endif()
    if(NOT TARGET SDL2_ttf::SDL2_ttf)
        add_library(SDL2_ttf::SDL2_ttf ALIAS PkgConfig::SDL2_MIXER)
    endif()

    # Linux 不需要 SDL2main
    add_library(SDL2::SDL2main INTERFACE IMPORTED)

    message(STATUS "SDL2 VERSION: ${SDL2_VERSION}")
    message(STATUS "SDL2_image VERSION: ${SDL2_IMAGE_VERSION}")
    message(STATUS "SDL2_mixer VERSION: ${SDL2_MIXER_VERSION}")
    message(STATUS "SDL2_ttf VERSION: ${SDL2_TTF_VERSION}")
  endif()
endif()

add_executable(th06_sdl2_test src/sdl2_test.cpp)

target_link_libraries(th06_sdl2_test PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)

if(WIN32)
    add_custom_command(TARGET th06_sdl2_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.dll"
            $<TARGET_FILE_DIR:th06_sdl2_test>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_IMAGE_DIR}/lib/${SDL2_ARCH}/SDL2_image.dll"
            $<TARGET_FILE_DIR:th06_sdl2_test>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.dll"
            $<TARGET_FILE_DIR:th06_sdl2_test>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_TTF_DIR}/lib/${SDL2_ARCH}/SDL2_ttf.dll"
            $<TARGET_FILE_DIR:th06_sdl2_test>
        COMMENT "复制 SDL2 DLL 到输出目录"
    )
endif()

set(TH06_GAME_SOURCES
    src/main.cpp
    src/GLFunc.cpp
    src/AnmManager.cpp
    src/AsciiManager.cpp
    src/BombData.cpp
    src/BulletData.cpp
    src/BulletManager.cpp
    src/Chain.cpp
    src/CMyFont.cpp
    src/Controller.cpp
    src/EclManager.cpp
    src/EffectManager.cpp
    src/Ending.cpp
    src/EnemyEclInstr.cpp
    src/EnemyManager.cpp
    src/FileSystem.cpp
    src/GameErrorContext.cpp
    src/GameManager.cpp
    src/GamePaths.cpp
    src/GameWindow.cpp
    src/Gui.cpp
    src/ItemManager.cpp
    src/MainMenu.cpp
    src/MidiOutput.cpp
    src/MusicRoom.cpp
    src/Player.cpp
    src/ReplayManager.cpp
    src/ResultScreen.cpp
    src/Rng.cpp
    src/ScreenEffect.cpp
    src/sdl2_renderer.cpp
    src/SoundPlayer.cpp
    src/Stage.cpp
    src/Supervisor.cpp
    src/TextHelper.cpp
    src/utils.cpp
    src/ZunTimer.cpp
#    src/zwave.cpp
    # graphics n midi
    src/midi/MidiDefault.cpp
    src/graphics/FixedFunctionGL.cpp
    src/graphics/WebGL.cpp
    # pbg3 portable
    src/pbg3/FileAbstraction.cpp
    src/pbg3/IPbg3Parser.cpp
    src/pbg3/Pbg3Archive.cpp
    src/pbg3/Pbg3Parser.cpp
    3rdparty/imgui/imgui.cpp
    3rdparty/imgui/imgui_draw.cpp
    3rdparty/imgui/imgui_tables.cpp
    3rdparty/imgui/imgui_widgets.cpp
    3rdparty/imgui/imgui_impl_sdl.cpp
    3rdparty/imgui/imgui_impl_opengl2.cpp
)

# Always compile both renderers and both ImGui GL backends for runtime switching
list(APPEND TH06_GAME_SOURCES 3rdparty/imgui/imgui_impl_opengl3.cpp)

add_executable(th06 ${TH06_GAME_SOURCES})

# Always define TH06_USE_GLES so both backends are available at runtime
target_compile_definitions(th06 PRIVATE TH06_USE_GLES=1)

# MSVC: Generate map file for debugging
if(MSVC)
    target_link_options(th06 PRIVATE /MAP)
endif()

# GCC/Clang: 游戏文件格式依赖32位指针大小，必须编译为32位
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(th06 PRIVATE
        -m32                     # 强制32位编译 (文件格式和结构体布局依赖sizeof(void*)==4)
        -fpermissive             # 允许GCC不支持的MSVC兼容语法 (额外限定名, 取右值地址等)
        -Wno-changes-meaning     # union成员名与类型名相同 (EclManager.hpp)
        -Wno-unknown-pragmas     # 忽略MSVC的 #pragma function / #pragma var_order
    )
    target_link_options(th06 PRIVATE -m32)
endif()

# 包含目录
target_include_directories(th06 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/build/autogenerated
    ${CMAKE_SOURCE_DIR}/3rdparty/rapidjson/include
    ${CMAKE_SOURCE_DIR}/3rdparty/imgui
)

# MSVC: i18n.hpp and thprac files contain UTF-8 CJK strings.
# Apply /utf-8 globally since i18n.hpp is included by many source files.
if(MSVC)
    target_compile_options(th06 PRIVATE /utf-8)
endif()

set(I18N_TPL "${CMAKE_SOURCE_DIR}/src/i18n.tpl")
set(I18N_HPP "${CMAKE_SOURCE_DIR}/build/autogenerated/i18n.hpp")

if(WIN32)
    set(TH06_PYTHON "python")
else()
    find_program(TH06_PYTHON NAMES python3 python REQUIRED)
endif()

set(TH_TEXT_ENCODING "utf-8")
message(STATUS "TH06 text encoding: ${TH_TEXT_ENCODING}")

add_custom_command(
    OUTPUT "${I18N_HPP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/build/autogenerated"
    COMMAND ${TH06_PYTHON} "${CMAKE_SOURCE_DIR}/scripts/gen_i18n.py"
        "${I18N_TPL}" "${I18N_HPP}" "${TH_TEXT_ENCODING}"
    DEPENDS "${I18N_TPL}" "${CMAKE_SOURCE_DIR}/scripts/gen_i18n.py"
    COMMENT "生成 i18n.hpp (UTF-8)"
)
add_custom_target(generate_i18n DEPENDS "${I18N_HPP}")
add_dependencies(th06 generate_i18n)

if(NOT WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    find_library(OPENGL_gl_LIBRARY NAMES GL PATHS /usr/lib/i386-linux-gnu NO_DEFAULT_PATH)
    if(NOT OPENGL_gl_LIBRARY)
        find_package(OpenGL REQUIRED)
    endif()
else()
    find_package(OpenGL REQUIRED)
endif()

target_link_libraries(th06 PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
)
if(OPENGL_gl_LIBRARY)
    target_link_libraries(th06 PRIVATE ${OPENGL_gl_LIBRARY})
else()
    target_link_libraries(th06 PRIVATE OpenGL::GL)
endif()

if(WIN32)
    target_link_libraries(th06 PRIVATE winmm)
endif()

if(WIN32)
    add_custom_command(TARGET th06 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_DIR}/lib/${SDL2_ARCH}/SDL2.dll"
            $<TARGET_FILE_DIR:th06>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_IMAGE_DIR}/lib/${SDL2_ARCH}/SDL2_image.dll"
            $<TARGET_FILE_DIR:th06>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_MIXER_DIR}/lib/${SDL2_ARCH}/SDL2_mixer.dll"
            $<TARGET_FILE_DIR:th06>
        COMMENT "复制 SDL2 DLL 到游戏输出目录"
    )
endif()

set(FONT_SRC "${CMAKE_SOURCE_DIR}/src/font/Noto Sans SC.ttf")
if(EXISTS "${FONT_SRC}")
    add_custom_command(TARGET th06 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:th06>/font"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FONT_SRC}"
            "$<TARGET_FILE_DIR:th06>/th06.ttf"
        COMMENT "Add Font"
    )
endif()
