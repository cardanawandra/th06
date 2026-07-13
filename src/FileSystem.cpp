#include <stdio.h>
#include <string.h>

#ifdef _XBOX
#include <xtl.h>
#elif defined(_WIN32)
#include <direct.h>
#include <new>
#include <windows.h>
#elif __cplusplus >= 201703L
#include <filesystem>
#else // Assume POSIX
#include <sys/stat.h>
#endif

#include "FileSystem.hpp"
#include "GamePaths.hpp"
#include "pbg3/Pbg3Archive.hpp"
#include "utils.hpp"
#include "compat/Compat.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __ANDROID__
#include <stdio.h>
#endif

u32 g_LastFileSize = 0;
bool g_LastFilePatched = false;

FILE *FileSystem::FopenUTF8(const char *filepath, const char *mode)
{
    char resolvedPath[1024];
    GamePaths::Resolve(resolvedPath, sizeof(resolvedPath), filepath);
    LOG_COMPAT("FileSystem::FopenUTF8 %s => %s\n",filepath,resolvedPath);
    LOG_COMPAT("FileSystem::FopenUTF8 ConvertToWide %s\n",resolvedPath);
     // First try native fopen().
    // On Japanese systems this handles Shift-JIS / CP932 correctly.
    FILE *f = fopen(resolvedPath, mode);

    if (f)
    {
        LOG_COMPAT("FileSystem::FopenUTF8 open success\n");
        return f;
    }
#ifdef COMPAT_PORTABLE
	return f;
#else
    LOG_COMPAT("FileSystem::FopenUTF8 open failed, fallback\n");
#ifndef MB_ERR_INVALID_CHARS
#define MB_ERR_INVALID_CHARS 0
#endif
    // Fallback: interpret filepath as UTF-8
    int filepathWLen =
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            resolvedPath,
            -1,
            NULL,
            0);

    int modeWLen =
        MultiByteToWideChar(
            CP_UTF8,
            0,
            mode,
            -1,
            NULL,
            0);

    if (filepathWLen <= 0 || modeWLen <= 0)
    {
        return NULL;
    }

    wchar_t *filepathW = new wchar_t[filepathWLen];
    wchar_t *modeW = new wchar_t[modeWLen];

    if (!MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            resolvedPath,
            -1,
            filepathW,
            filepathWLen))
    {
        delete[] filepathW;
        delete[] modeW;
        return NULL;
    }

    MultiByteToWideChar(
        CP_UTF8,
        0,
        mode,
        -1,
        modeW,
        modeWLen);

    f = _wfopen(filepathW, modeW);

    delete[] filepathW;
    delete[] modeW;

    LOG_COMPAT("FileSystem::FopenUTF8 fallback success\n");
    return f;
#endif
}

void FileSystem::CreateDir(const char *path)
{
    char resolvedPath[1024];
    GamePaths::Resolve(resolvedPath, sizeof(resolvedPath), path);
#if defined(__ANDROID__)
    mkdir(resolvedPath,0755);
#elif defined(_XBOX)
//    CreateDirectory(resolvedPath, NULL);
#elif defined(_WIN32)
    _mkdir(path);
#endif
}

void str_replace(char *str, const char *from, const char *to)
{
    char *pos = strstr(str, from);
    if (!pos)
        return;

    size_t fromLen = strlen(from);
    size_t toLen = strlen(to);

    memmove(pos + toLen,
            pos + fromLen,
            strlen(pos + fromLen) + 1);

    memcpy(pos, to, toLen);
}

u8 *FileSystem::OpenPath(const char *filepath, int isExternalResource)
{
    u8 *data;
    FILE *file;
    size_t fsize;
    i32 entryIdx;
    const char *entryname;
    i32 pbg3Idx;
    LOG_COMPAT("FileSystem::OpenPath 1\n");

    char resolvedPath[512];
    GamePaths::Resolve(resolvedPath, sizeof(resolvedPath), filepath);
    LOG_COMPAT("FileSystem::OpenPath src %s\n",resolvedPath);

    entryIdx = -1;
    if (isExternalResource == 0)
    {
        LOG_COMPAT("FileSystem::OpenPath isExternalResource == 0\n");
        entryname = strrchr(filepath, '\\');
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        LOG_COMPAT("FileSystem::OpenPath entryname = %s\n",entryname);
        entryname = strrchr(entryname, '/');
        LOG_COMPAT("FileSystem::OpenPath entryname changed into = %s\n",entryname);
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        // PATCH FIRST
        char patchPath[512];
        char patchPathRaw[512];
        SNPRINTF(patchPathRaw, sizeof(patchPathRaw), "patch/%s", entryname);
        GamePaths::Resolve(patchPath, sizeof(patchPath), patchPathRaw);
        file = fopen(patchPath, "rb");
        if (file != NULL)
        {
            printf("FileSystem::OpenPath patch fopen %s\n",patchPath);
            fseek(file, 0, SEEK_END);
            fsize = ftell(file);
            g_LastFileSize = fsize;
            fseek(file, 0, SEEK_SET);
            data = (u8 *)malloc(fsize);
            fread(data, 1, fsize, file);
            fclose(file);
            g_LastFilePatched = true;
            return data;
        }
        g_LastFilePatched = false;
        // printf("FileSystem::OpenPath no patch %s\n",patchPath);

        if (g_Pbg3Archives != NULL)
        {
            LOG_COMPAT("FileSystem::OpenPath g_Pbg3Archives exists\n");
            for (pbg3Idx = 0; pbg3Idx < 0x10; pbg3Idx += 1)
            {
                if (g_Pbg3Archives[pbg3Idx] != NULL)
                {
                    LOG_COMPAT("FileSystem::OpenPath g_Pbg3Archives[pbg3Idx]->FindEntry(%s)\n",entryname);
                    entryIdx = g_Pbg3Archives[pbg3Idx]->FindEntry(entryname);
                    if (entryIdx >= 0)
                    {
                        break;
                    }
                }
            }
        }
        if (entryIdx < 0)
        {
            LOG_COMPAT("FileSystem::OpenPath entry not found\n");
            return NULL;
        }
    }
    if (entryIdx >= 0)
    {
        LOG_COMPAT("FileSystem::OpenPath g_Pbg3Archives[pbg3Idx]->ReadDecompressEntry\n",entryname);
        data = g_Pbg3Archives[pbg3Idx]->ReadDecompressEntry(entryIdx, entryname);
        LOG_COMPAT("FileSystem::OpenPath g_Pbg3Archives[pbg3Idx]->GetEntrySize\n",entryname);
        g_LastFileSize = g_Pbg3Archives[pbg3Idx]->GetEntrySize(entryIdx);
    }
    else
    {
        LOG_COMPAT("FileSystem::OpenPath fopen %s\n",resolvedPath);
        file = fopen(resolvedPath, "rb");
        if (file == NULL)
        {
            LOG_COMPAT("FileSystem::OpenPath file not found %s\n",resolvedPath);
            return NULL;
        }
        else
        {
            fseek(file, 0, SEEK_END);
            fsize = ftell(file);
            g_LastFileSize = fsize;
            fseek(file, 0, SEEK_SET);
            data = (u8 *)malloc(fsize);
            fread(data, 1, fsize, file);
            fclose(file);
        }
    }
    LOG_COMPAT("FileSystem::OpenPath success\n");
    return data;
}

int FileSystem::WriteDataToFile(const char *path, const void *data, size_t size)
{
	#if COMPAT_UNWRITABLE
		return 0;
	#else
    FILE *f;

    char resolvedPath[512];
    GamePaths::Resolve(resolvedPath, sizeof(resolvedPath), path);
    f = fopen(resolvedPath, "wb");

    if (f == NULL)
    {
        return -1;
    }
    else
    {
        if (fwrite(data, 1, size, f) != size)
        {
            fclose(f);
            return -2;
        }
        else
        {
            fclose(f);
            return 0;
        }
    }
	#endif
}