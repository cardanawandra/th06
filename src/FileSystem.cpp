#include <stdio.h>
#include <string.h>

#ifdef _WIN32
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
#ifdef __ANDROID__
#include <SDL.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#endif

u32 g_LastFileSize = 0;

FILE *FileSystem::FopenUTF8(const char *filepath, const char *mode)
{
#ifdef __ANDROID__

    std::string resolvedPath = std::string(GamePaths::GetUserPath()) + filepath;
    return fopen(resolvedPath.c_str(), mode);

#else

#ifndef _WIN32
    //printf("FileSystem::FopenUTF8 _WIN32 not defined for UTF8\n");
    return fopen(filepath, mode);

#else

    //printf("FileSystem::FopenUTF8 ConvertToWide %s\n",filepath);
     // First try native fopen().
    // On Japanese systems this handles Shift-JIS / CP932 correctly.
    FILE *f = fopen(filepath, mode);

    if (f)
    {
        //printf("FileSystem::FopenUTF8 open success\n");
        return f;
    }
    //printf("FileSystem::FopenUTF8 open failed, fallback\n");

    // Fallback: interpret filepath as UTF-8
    int filepathWLen =
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            filepath,
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
            filepath,
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

    //printf("FileSystem::FopenUTF8 fallback success\n");
    return f;

#endif
#endif
}

void FileSystem::CreateDir(const char *path)
{
#ifdef __ANDROID__
    string resolvedPath = string(GamePaths::GetUserPath()) + string(path);
    mkdir(resolvedPath.c_str(),0755);
#else
#ifdef _WIN32
    _mkdir(path);
#elif __cplusplus >= 201703L
    auto p = filesystem::path(path);
    filesystem::create_directory(p);
#else
    mkdir(path, 0755);
#endif
#endif
}

u8 *FileSystem::OpenPath(const char *filepath, int isExternalResource)
{
    u8 *data;
    FILE *file;
    size_t fsize;
    i32 entryIdx;
    const char *entryname;
    i32 pbg3Idx;
    //printf("FileSystem::OpenPath 1\n");

    #ifdef __ANDROID__
    string resolvedPath = string(GamePaths::GetUserPath()) + string(filepath);
    #else
    char resolvedPath[512];
    GamePaths::Resolve(resolvedPath, sizeof(resolvedPath), filepath);
    #endif
    //printf("FileSystem::OpenPath src %s\n",resolvedPath);

    entryIdx = -1;
    if (isExternalResource == 0)
    {
        //printf("FileSystem::OpenPath isExternalResource == 0\n");
        entryname = strrchr(filepath, '\\');
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        //printf("FileSystem::OpenPath entryname = %s\n",entryname);
        entryname = strrchr(entryname, '/');
        //printf("FileSystem::OpenPath entryname changed into = %s\n",entryname);
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        if (g_Pbg3Archives != NULL)
        {
            //printf("FileSystem::OpenPath g_Pbg3Archives exists\n");
            for (pbg3Idx = 0; pbg3Idx < 0x10; pbg3Idx += 1)
            {
                if (g_Pbg3Archives[pbg3Idx] != NULL)
                {
                    //printf("FileSystem::OpenPath g_Pbg3Archives[pbg3Idx]->FindEntry(%s)\n",entryname);
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
            //printf("FileSystem::OpenPath entry not found\n");
            return NULL;
        }
    }
    if (entryIdx >= 0)
    {
        //printf("FileSystem::OpenPath g_Pbg3Archives[pbg3Idx]->ReadDecompressEntry\n",entryname);
        data = g_Pbg3Archives[pbg3Idx]->ReadDecompressEntry(entryIdx, entryname);
        //printf("FileSystem::OpenPath g_Pbg3Archives[pbg3Idx]->GetEntrySize\n",entryname);
        g_LastFileSize = g_Pbg3Archives[pbg3Idx]->GetEntrySize(entryIdx);
    }
    else
    {
        #ifdef __ANDROID__
        file = fopen(resolvedPath.c_str(), "rb");
        #else
        //printf("FileSystem::OpenPath fopen %s\n",resolvedPath);
        file = fopen(resolvedPath, "rb");
        #endif
        if (file == NULL)
        {
            //printf("FileSystem::OpenPath file not found %s\n",resolvedPath);
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
    //printf("FileSystem::OpenPath success\n");
    return data;
}

int FileSystem::WriteDataToFile(const char *path, const void *data, size_t size)
{
    FILE *f;

    #ifdef __ANDROID__
    string resolvedPath = string(GamePaths::GetUserPath()) + string(path);
    f = fopen(resolvedPath.c_str(), "wb");
    #else
    // Resolve to writable user-data directory on Android.
    char resolvedPath[512];
    GamePaths::Resolve(resolvedPath, sizeof(resolvedPath), path);
    GamePaths::EnsureParentDir(resolvedPath);
    f = fopen(resolvedPath, "wb");
    #endif

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
}