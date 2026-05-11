[![Discord][discord-badge]][discord] <- click here to join discord server.

[discord]: https://discord.gg/VyGwAjrh9a
[discord-badge]: https://img.shields.io/discord/1147558514840064030?color=%237289DA&logo=discord&logoColor=%23FFFFFF

This is the readme for the crossplatform fork of EoSD. For the readme of the decomp project, see [here](https://github.com/GensokyoClub/th06/blob/master/README.md).

EoSD-crossplatform is a port of Touhou 6 using SDL and OpenGL (with a more general renderer abstraction layer hopefully on the way).
This enables theoretical portability to any system supported by SDL, with Linux and Windows in particular being known to work.
Builds for OS X, the BSDs, and other Unices are also almost certainly possible, but may require some slight modifications to the build system.

### Platform Requirements

- SDL, SDL-image, and SDL-mixer support<br>
- C++98 standard library support<br>
- OpenGL ES 1.1, OpenGL 1.3, or GL ES 2.0 / WebGL support

### Dependencies

EoSD-crossplatform has the following dependencies:

- `SDL-1.2.15`
- `SDL_image-1.2.5`
- `SDL_ttf-2.0.4`

#### Building Windows
building uses CMAKE 3.20 and a compiler that supports C++20 like msvc17.<br>

run "cmake -B build_sdl -A Win32"<br>
if success then<br>
run "cmake --build build_sdl --config Release"<br>
<br>
result will be on "build_sdl\Release"

#### Building Windows
building uses CMAKE 2.4 and a compiler that supports C++98 like msvc6.<br>
download msvc6 on https://github.com/itsmattkc/MSVC600<br>
change the bat file from E:\library\msvc6\MSVC600-master\Common\MSDev98\Bin\MSDEV.EXE into your own path

run "cd pc98"<br>
run "initCMake.bat"<br>
if success then<br>
run "buildRelease.bat"<br>
<br>
result are exe only, will be on "pc98\Release", just copy th06.exe into build_sdl\Release
<br>
cleanup? <br>
run "removeCache.bat"<br>
<br>
#### Building Android (TODO with SDL1.2)
-download source SDL and place it in android/SDL_SRC<br>
-download source SDL-image and place it in android/SDL_image_SRC<br>
-download source SDL-ttf and place it in android/SDL_ttf_SRC<br>
-set your sdk dir in android/local.properties ("sdk.dir=your android sdk path")<br>
-run "cd android"<br>
-then "./gradlew clean assembleDebug" (for windows "gradlew.bat clean assembleDebug")<br>
<br>
result will be on "android\app\build\outputs\apk\debug\app_debug.apk"

### Use

EoSD-crossplatform is designed to be a drop-in replacement for the vanilla EoSD binary for android.

A Japanese locale is not required.

# Decomp Credits

special thanks to
- Team Shanghai Alice for the games
- Gensokyo Club for decompilation
- toadster172 for portable branch
- CNTianQi233 for android skeleton
- 32th system for portable iconv

We would like to extend our thanks to the following individuals for their
invaluable contributions:

- @EstexNT for porting the [`var_order` pragma](scripts/pragma_var_order.cpp) to
  MSVC7.