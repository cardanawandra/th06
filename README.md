[![Discord][discord-badge]][discord] <- click here to join discord server.

[discord]: https://discord.gg/VyGwAjrh9a
[discord-badge]: https://img.shields.io/discord/1147558514840064030?color=%237289DA&logo=discord&logoColor=%23FFFFFF

This is the readme for the crossplatform fork of EoSD. For the readme of the decomp project, see [here](https://github.com/GensokyoClub/th06/blob/master/README.md).

EoSD-crossplatform is a port of Touhou 6 using SDL2 and OpenGL (with a more general renderer abstraction layer hopefully on the way).
This enables theoretical portability to any system supported by SDL2, with Linux and Windows in particular being known to work.
Builds for OS X, the BSDs, and other Unices are also almost certainly possible, but may require some slight modifications to the build system.

### Platform Requirements

- SDL2.30.0, SDL2-image, and SDL2-mixer support<br>
- C++17 standard library support<br>
- OpenGL ES 1.1, OpenGL 1.3, or GL ES 2.0 / WebGL support

### Dependencies

EoSD-portable has the following dependencies:

- `SDL2.30`
- `SDL2_image`
- `SDL2_mixer`

In addition, building uses CMAKE 3.18 and a compiler that supports C++17.

#### Building Windows
run "cmake -B build_sdl2 -A Win32 -DUSE_GLES=ON"<br>
if success then<br>
run "cmake --build build_sdl2 --config Release"<br>
<br>
result will be on "build_sdl2\Release"

#### Building Android
-download source SDL2.30.0 and place it in android/SDL2_SRC<br>
-download source SDL2-mixer and place it in android/SDL2_MIXER_SRC<br>
-download source SDL2-image and place it in android/SDL2_image_SRC<br>
-set your sdk dir in android/local.properties ("sdk.dir=your android sdk path")<br>
-run "cd android"<br>
-then "./gradlew clean assembleDebug" (for windows "gradlew.bat clean assembleDebug")<br>
<br>
result will be on "android\app\build\outputs\apk\debug\app_debug.apk"

### Use

EoSD-crossplatform is designed to be a drop-in replacement for the vanilla EoSD binary.

A Japanese locale is not required.

# Decomp Credits

special thanks to
- Team Shanghai Alice for the games
- Gensokyo Club for decompilation
- toadster172 for portable branch
- CNTianQi233 for android skeleton

We would like to extend our thanks to the following individuals for their
invaluable contributions:

- @EstexNT for porting the [`var_order` pragma](scripts/pragma_var_order.cpp) to
  MSVC7.