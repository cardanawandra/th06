#include "GamePaths.hpp"

#include <SDL.h>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace GamePaths
{

static char s_userPath[512] = "";

void Init()
{
#ifdef __ANDROID__
    const char *internalPath = SDL_AndroidGetExternalStoragePath();
    if (internalPath)
    {
        snprintf(s_userPath, sizeof(s_userPath), "%s/", internalPath);
        SDL_Log("GamePaths: user data path = %s", s_userPath);
    }
    else
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "GamePaths: SDL_AndroidGetExternalStoragePath() returned NULL, using cwd");
        s_userPath[0] = '\0';
    }
#else
    // Desktop: all files relative to the working directory.
    s_userPath[0] = '\0';
#endif
}

const char *GetUserPath()
{
    return s_userPath;
}

bool IsAssetPath(const char *path)
{
    if (!path || !*path)
        return false;

    // Paths starting with these prefixes are read-only game assets:
    if (strncmp(path, "data/", 5) == 0 || strncmp(path, "data\\", 5) == 0)
        return true;
    if (strncmp(path, "bgm/", 4) == 0 || strncmp(path, "bgm\\", 4) == 0)
        return true;
    if (strncmp(path, "font/", 5) == 0 || strncmp(path, "font\\", 5) == 0)
        return true;

    // Any .dat file (pbg3 archives like紅魔郷IN.dat)
    const char *dot = strrchr(path, '.');
    if (dot)
    {
#ifdef _WIN32
        if (_stricmp(dot, ".dat") == 0)
            return true;
#else
        if (strcasecmp(dot, ".dat") == 0)
            return true;
#endif
    }

    return false;
}

void Resolve(char *outBuf, size_t outBufSize, const char *path)
{
    if (!path || !*path)
    {
        outBuf[0] = '\0';
        return;
    }

    // Strip leading "./" or ".\\"
    if (path[0] == '.' && (path[1] == '/' || path[1] == '\\'))
        path += 2;

    if (IsAssetPath(path))
    {
        // Asset: keep the relative path as-is.
        // SDL_RWFromFile on Android reads from APK assets/ automatically.
        snprintf(outBuf, outBufSize, "%s", path);
    }
    else
    {
        // User data: prepend the writable user-data directory.
        snprintf(outBuf, outBufSize, "%s%s", s_userPath, path);
    }
}

void EnsureParentDir(const char *resolvedPath)
{
    // Find the last directory separator and create the directory.
    char dirBuf[512];
    snprintf(dirBuf, sizeof(dirBuf), "%s", resolvedPath);

    char *lastSep = strrchr(dirBuf, '/');
#ifdef _WIN32
    {
        char *lastBs = strrchr(dirBuf, '\\');
        if (lastBs && (!lastSep || lastBs > lastSep))
            lastSep = lastBs;
    }
#endif

    if (lastSep && lastSep != dirBuf)
    {
        *lastSep = '\0';
#ifdef _WIN32
        _mkdir(dirBuf);
#else
        mkdir(dirBuf, 0755);
#endif
    }
}

} // namespace GamePaths
