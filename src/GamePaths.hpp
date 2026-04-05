#pragma once
// =============================================================================
// GamePaths.hpp — Platform-specific file path routing
//
// On most platforms, all files live in the working directory next to the exe.
// On Android:
//   - Read-only assets (.dat, font/, bgm/, data/) → SDL_RWFromFile reads from
//     APK assets/ automatically when given a relative path.
//   - User data (.cfg, score.dat, replay/) → SDL_AndroidGetInternalStoragePath()
//     provides a writable directory per-app.
//
// Usage:
//   GamePaths::Init();                          // call once at startup
//   const char *p = GamePaths::GetUserPath();   // "" or "/data/data/.../files/"
//   bool isAsset = GamePaths::IsAssetPath(path);
//   char resolved[512];
//   GamePaths::Resolve(resolved, sizeof(resolved), "score.dat");
//     → on desktop: "score.dat"
//     → on Android: "/data/data/com.th06/files/score.dat"
//   GamePaths::Resolve(resolved, sizeof(resolved), "bgm/th06_01.wav");
//     → on desktop: "bgm/th06_01.wav"
//     → on Android: "bgm/th06_01.wav"   (unchanged — SDL reads from assets)
// =============================================================================

#include <SDL.h>

namespace GamePaths
{

// Must be called once before any file I/O (e.g. at the start of main()).
void Init();

// Returns the writable user-data directory (with trailing separator).
// On desktop: returns "" (current working directory).
// On Android: returns SDL_AndroidGetInternalStoragePath() + "/".
const char *GetUserPath();

// Returns true if `path` should be read from the read-only asset store
// (APK assets on Android, working dir on desktop).
// Asset paths: anything starting with "data/", "bgm/", "font/", or
// any path containing a .dat extension (the pbg3 archives).
bool IsAssetPath(const char *path);

// Resolve `path` into `outBuf`.
// If IsAssetPath(path), copies the path unchanged (SDL_RWFromFile reads assets).
// Otherwise, prepends GetUserPath().
void Resolve(char *outBuf, size_t outBufSize, const char *path);

// Convenience: ensure the user-data directory for a given path exists.
// E.g. for "replay/th6_01.rpy", creates GetUserPath() + "replay/".
void EnsureParentDir(const char *resolvedPath);

} // namespace GamePaths