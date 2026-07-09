#include "GamePaths.hpp"
#include "inttypes.hpp"

#include <stdio.h>
#include <cstring>
#include <cstddef>
#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
#include "compat/Compat.hpp"

namespace GamePaths
{

static char s_userPath[512] = "";

void Init()
{
    // On Android, SDL must be initialized before GamePaths::Init()
    STORAGE_INIT();
    const char *internalPath = GET_EXTERNAL_STORAGE_PATH();
    if (internalPath)
    {
        SNPRINTF(s_userPath, sizeof(s_userPath), "%s/", internalPath);
        LOG_COMPAT("GamePaths: user data path = %s/", s_userPath);
    }
    else
    {
        s_userPath[0] = '\0';
    }
}

const char *GetUserPath()
{
    return s_userPath;
}

bool IsAssetPath(const char *path)
{
    return false;
    if (!path || !*path)
        return false;

    // Paths starting with these prefixes are read-only game assets:
    if (strncmp(path, "data/", 5) == 0 || strncmp(path, "data\\", 5) == 0)
        return true;
    if (strncmp(path, "bgm/", 4) == 0 || strncmp(path, "bgm\\", 4) == 0)
        return true;
    if (strncmp(path, "font/", 5) == 0 || strncmp(path, "font\\", 5) == 0)
        return true;

    return false;
}
void Resolve(char *outBuf, int outBufSize, const char *path)
{
	SNPRINTF(outBuf, outBufSize, "%s%s", s_userPath, path);
	for (char* p = outBuf; *p; ++p)
	{
		if (*p == '\\')
			*p = '/';
	}
	return;
}

void EnsureParentDir(const char *resolvedPath)
{
    // Find the last directory separator and create the directory.
    char dirBuf[512];
    SNPRINTF(dirBuf, sizeof(dirBuf), "%s", resolvedPath);

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
