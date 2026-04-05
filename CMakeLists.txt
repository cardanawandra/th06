###############################################################################
# th06 - 東方紅魔郷 SDL2 移植版构建脚本
#
# 用法 (Windows x86):
#   cmake -B build_sdl2 -A Win32
#   cmake --build build_sdl2 --config Release
#
# 用法 (Windows x64):
#   cmake -B build_sdl2 -A x64
#   cmake --build build_sdl2 --config Release
#
# 用法 (Linux 32-bit, 在 WSL 或原生 Linux 中):
#   PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig cmake -B build_linux -DCMAKE_BUILD_TYPE=Release
#   cmake --build build_linux --target th06
#   (需要: sudo apt install gcc-multilib g++-multilib libsdl2-dev:i386 libsdl2-image-dev:i386 libsdl2-mixer-dev:i386 libgl-dev:i386)
#
# 用法 (Android, 需要 NDK):
#   cmake -B build_android -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a
#   cmake --build build_android
###############################################################################

cmake_minimum_required(VERSION 3.20)
project(th06_sdl2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Option: build with OpenGL ES 2.0 shader-based renderer instead of fixed-function GL
option(USE_GLES "Build with OpenGL ES 2.0 renderer (RendererGLES) instead of fixed-function RendererGL" OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# 选项
# ============================================================================
option(TH06_USE_SDL2 "使用 SDL2 作为平台后端" ON)
option(TH06_BUILD_ORIGINAL "构建原始 D3D8/Win32 版本 (仅限 Windows)" OFF)

# ============================================================================
# SDL2 查找
# Windows: 使用项目内置的 3rdparty 版本
# Linux/macOS: 使用系统安装的 SDL2 (pkg-config)
# ============================================================================
if(TH06_USE_SDL2)
  if(WIN32)
    # Windows: 使用项目自带的预编译 SDL2 库
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_ARCH "x64")
    else()
        set(SDL2_ARCH "x86")
    endif()

    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2")
    set(SDL2_IMAGE_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2_image")
    set(SDL2_MIXER_DIR "${CMAKE_SOURCE_DIR}/3rdparty/SDL2_mixer")

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

    message(STATUS "SDL2 路径: ${SDL2_DIR}")
    message(STATUS "SDL2_image 路径: ${SDL2_IMAGE_DIR}")
    message(STATUS "SDL2_mixer 路径: ${SDL2_MIXER_DIR}")
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

    # Linux 不需要 SDL2main
    add_library(SDL2::SDL2main INTERFACE IMPORTED)

    message(STATUS "SDL2 (系统): ${SDL2_VERSION}")
    message(STATUS "SDL2_image (系统): ${SDL2_IMAGE_VERSION}")
    message(STATUS "SDL2_mixer (系统): ${SDL2_MIXER_VERSION}")
  endif()
endif()

# ============================================================================
# SDL2 验证测试程序
# ============================================================================
add_executable(th06_sdl2_test src/sdl2_test.cpp)

target_link_libraries(th06_sdl2_test PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
)

# Windows: 复制 DLL 到测试程序输出目录
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
        COMMENT "复制 SDL2 DLL 到输出目录"
    )
endif()

# ============================================================================
# 主游戏可执行文件 th06
# ============================================================================

# 收集源文件 (排除原版构建专用文件和测试文件)
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
    # pbg3 文件解析库
    src/pbg3/FileAbstraction.cpp
    src/pbg3/IPbg3Parser.cpp
    src/pbg3/Pbg3Archive.cpp
    src/pbg3/Pbg3Parser.cpp
    # thprac 练习模式
    # src/thprac_th06.cpp
    # src/thprac_games.cpp
    # src/thprac_gui_components.cpp
    # src/thprac_gui_locale.cpp
    # src/thprac_gui_input.cpp
    # src/thprac_locale_def.cpp
    # src/thprac_gui_integration.cpp
    # ImGui 核心源文件
    3rdparty/imgui/imgui.cpp
    3rdparty/imgui/imgui_draw.cpp
    3rdparty/imgui/imgui_tables.cpp
    3rdparty/imgui/imgui_widgets.cpp
    # ImGui SDL2+OpenGL2 后端
    3rdparty/imgui/imgui_impl_sdl.cpp
    3rdparty/imgui/imgui_impl_opengl2.cpp
)

# Always compile both renderers and both ImGui GL backends for runtime switching
#list(APPEND TH06_GAME_SOURCES src/RendererGLES.cpp)
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

# i18n.hpp 生成 (UTF-8 → 系统编码)
# 自动检测系统ANSI代码页，将i18n字符串编码为匹配的编码格式
# 非ASCII字符(如♪)通过hex escape处理，避免编码兼容问题
set(I18N_TPL "${CMAKE_SOURCE_DIR}/src/i18n.tpl")
set(I18N_HPP "${CMAKE_SOURCE_DIR}/build/autogenerated/i18n.hpp")

# 查找 python 可执行文件 (Windows: python, Linux: python3)
if(WIN32)
    set(TH06_PYTHON "python")
else()
    find_program(TH06_PYTHON NAMES python3 python REQUIRED)
endif()

# i18n.hpp is always generated as UTF-8.
# Runtime auto-detection in TextHelper.cpp handles GBK / Shift-JIS game data.
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

# 查找 OpenGL
if(NOT WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # 32位编译: 手动指定i386 OpenGL库路径, 避免find_package找到64位版本
    find_library(OPENGL_gl_LIBRARY NAMES GL PATHS /usr/lib/i386-linux-gnu NO_DEFAULT_PATH)
    if(NOT OPENGL_gl_LIBRARY)
        find_package(OpenGL REQUIRED)
    endif()
else()
    find_package(OpenGL REQUIRED)
endif()

# 链接库
target_link_libraries(th06 PRIVATE
    SDL2::SDL2main
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
)
if(OPENGL_gl_LIBRARY)
    target_link_libraries(th06 PRIVATE ${OPENGL_gl_LIBRARY})
else()
    target_link_libraries(th06 PRIVATE OpenGL::GL)
endif()

# Windows 特定: Winmm (timeGetTime 兼容)
if(WIN32)
    target_link_libraries(th06 PRIVATE winmm)
endif()

# Windows: 复制 DLL 到主游戏输出目录
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

# 如果 src/font/Noto Sans SC.ttf 存在，自动部署到游戏输出目录的 font/ 文件夹
set(FONT_SRC "${CMAKE_SOURCE_DIR}/src/font/Noto Sans SC.ttf")
if(EXISTS "${FONT_SRC}")
    add_custom_command(TARGET th06 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:th06>/font"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FONT_SRC}"
            "$<TARGET_FILE_DIR:th06>/font/Noto Sans SC.ttf"
        COMMENT "部署字体文件到游戏目录"
    )
endif()
