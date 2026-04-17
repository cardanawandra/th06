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
#include <cstdio>
#endif

u32 g_LastFileSize = 0;

FILE *FileSystem::FopenUTF8(const char *filepath, const char *mode)
{
#ifdef __ANDROID__
    std::string resolvedPath = std::string(GamePaths::GetUserPath()) + std::string(filepath);
    return std::fopen(resolvedPath.c_str(), mode);
#else
#ifndef _WIN32
    return std::fopen(filepath, mode);
#else
    u32 filepathWLen = MultiByteToWideChar(CP_UTF8, 0, filepath, -1, NULL, 0) * 2;
    u32 modeWLen = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0) * 2;

    if (filepathWLen == 0 || modeWLen == 0)
    {
        return NULL;
    }

    wchar_t *filepathW = new wchar_t[filepathWLen];
    wchar_t *modeW = new wchar_t[modeWLen];

    MultiByteToWideChar(CP_UTF8, 0, filepath, -1, filepathW, filepathWLen / 2);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, modeW, modeWLen / 2);

    FILE *f = _wfopen(filepathW, modeW);

    delete[] filepathW;
    delete[] modeW;

    return f;
#endif
#endif
}

void FileSystem::CreateDir(const char *path)
{
#ifdef __ANDROID__
    std::string resolvedPath = std::string(GamePaths::GetUserPath()) + std::string(path);
    mkdir(resolvedPath.c_str(),0755);
#else
#ifdef _WIN32
    _mkdir(path);
#elif __cplusplus >= 201703L
    auto p = std::filesystem::path(path);
    std::filesystem::create_directory(p);
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

    #ifdef __ANDROID__
    std::string resolvedPath = std::string(GamePaths::GetUserPath()) + std::string(filepath);
    #else
    char resolvedPath[512];
    GamePaths::Resolve(resolvedPath, sizeof(resolvedPath), filepath);
    #endif

    entryIdx = -1;
    if (isExternalResource == 0)
    {
        entryname = strrchr(filepath, '\\');
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        entryname = strrchr(entryname, '/');
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
            for (pbg3Idx = 0; pbg3Idx < 0x10; pbg3Idx += 1)
            {
                if (g_Pbg3Archives[pbg3Idx] != NULL)
                {
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
            return NULL;
        }
    }
    if (entryIdx >= 0)
    {
        utils::DebugPrint2("%s Decode ... \n", entryname);
        data = g_Pbg3Archives[pbg3Idx]->ReadDecompressEntry(entryIdx, entryname);
        g_LastFileSize = g_Pbg3Archives[pbg3Idx]->GetEntrySize(entryIdx);
    }
    else
    {
        #ifdef __ANDROID__
        file = fopen(resolvedPath.c_str(), "rb");
        #else
        utils::DebugPrint2("%s Load ... \n", resolvedPath);
        file = fopen(resolvedPath, "rb");
        #endif
        if (file == NULL)
        {
            #ifndef __ANDROID__
            utils::DebugPrint2("error : %s is not found.\n", resolvedPath);
            #endif
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
    return data;
}

int FileSystem::WriteDataToFile(const char *path, void *data, size_t size)
{
    FILE *f;

    #ifdef __ANDROID__
    std::string resolvedPath = std::string(GamePaths::GetUserPath()) + std::string(path);
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