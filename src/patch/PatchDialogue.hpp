#pragma once
#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>

#include "../thirdparty/json.h"
#include "../FileSystem.hpp"

struct PatchDialogueEntry
{
    std::vector<std::string> lines;
};

struct PatchDialogueSection
{
    std::vector<std::pair<std::string, PatchDialogueEntry> > entries;
};

struct PatchDialogue
{
    std::map<std::string, PatchDialogueSection> sections;
};

inline PatchDialogueEntry LoadPatchDialogueEntry(json_object_s *object)
{
    PatchDialogueEntry entry;

    for (json_object_element_s *field = object->start;
         field;
         field = field->next)
    {
        if (strcmp(field->name->string, "lines") == 0)
        {
            json_array_s *array = json_value_as_array(field->value);

            for (json_array_element_s *line = array->start;
                 line;
                 line = line->next)
            {
                json_string_s *str = json_value_as_string(line->value);
                if (str)
                    entry.lines.push_back(str->string);
            }
        }
    }

    return entry;
}

inline PatchDialogueSection LoadPatchDialogueSection(json_object_s *object)
{
    PatchDialogueSection section;

    for (json_object_element_s *entry = object->start;
         entry;
         entry = entry->next)
    {
        json_object_s *dialogue = json_value_as_object(entry->value);
        if (!dialogue)
            continue;

        section.entries.push_back(
            std::make_pair(entry->name->string,
                   LoadPatchDialogueEntry(dialogue)));
    }

    return section;
}

inline json_value_s *LoadJsonFile(const char *filename)
{
    u8 *data = FileSystem::OpenPath(filename, 0);
    printf("LoadJsonFile filename : %s\n",filename);
    if (!data)
        return NULL;

    printf("LoadJsonFile get : %s\n",filename);
    json_value_s *root = json_parse(
        (const char *)data,
        g_LastFileSize);

    free(data);

    return root;
}

inline bool LoadPatchDialogueSection(
    const char *filename,
    PatchDialogueSection *section)
{
    if (!section)
        return false;

    json_value_s *root = LoadJsonFile(filename);
    if (!root)
        return false;

    json_object_s *rootObject = json_value_as_object(root);
    if (!rootObject)
    {
        free(root);
        return false;
    }

    *section = LoadPatchDialogueSection(rootObject);

    free(root);
    return true;
}

inline bool LoadPatchDialogue(
    const char *filename,
    PatchDialogue *patch)
{
    printf("LoadPatchDialogue 1\n");
    if (!patch)
        return false;

    printf("LoadPatchDialogue 2\n");
    json_value_s *root = LoadJsonFile(filename);
    if (!root)
        return false;

    printf("LoadPatchDialogue 3\n");
    json_object_s *rootObject = json_value_as_object(root);
    if (!rootObject)
    {
        free(root);
        return false;
    }

    printf("LoadPatchDialogue 4\n");
    for (json_object_element_s *section = rootObject->start;
         section;
         section = section->next)
    {
        (*patch).sections[section->name->string] =
            LoadPatchDialogueSection(
                json_value_as_object(section->value));
    }

    printf("LoadPatchDialogue 5\n");
    free(root);
    return true;
}

inline std::string SanitizeDialogueLine(std::string line)
{
    // Remove TL notes.
    size_t tl = line.find("TL note:");
    if (tl != std::string::npos)
        line.erase(tl);

    // Remove everything after 0x14.
    size_t ctrl = line.find('\x14');
    if (ctrl != std::string::npos)
        line.erase(ctrl);

    // Convert <c$A$B> -> A\nB
    size_t p = line.find("<c$");
    while (p != std::string::npos)
    {
        size_t mid = line.find('$', p + 3);
        size_t end = line.find('>', mid + 1);

        if (mid == std::string::npos || end == std::string::npos)
            break;

        line.replace(p, end - p + 1,
            line.substr(p + 3, mid - (p + 3)) + "\n" +
            line.substr(mid + 1, end - (mid + 1)));

        p = line.find("<c$", p);
    }

    return line;
}

inline void BuildPatchDialogueArray(
    const PatchDialogue &patch,
    const char *sectionNames,
    std::vector<char *> *out,
    std::vector<char *> *outHeader)
{
    out->clear();
    outHeader->clear();

    char buffer[256];
    strncpy(buffer, sectionNames, sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';

    char *sectionName = strtok(buffer, "-");

    while (sectionName)
    {
        std::map<std::string, PatchDialogueSection>::const_iterator sec =
            patch.sections.find(sectionName);

        if (sec != patch.sections.end())
        {
            const PatchDialogueSection &section = sec->second;

            for (size_t i = 0; i < section.entries.size(); ++i)
            {
                const std::string &key = section.entries[i].first;
                const PatchDialogueEntry &entry = section.entries[i].second;

                std::string merged;

                for (size_t j = 0; j < entry.lines.size(); ++j)
                {
                    std::string line = SanitizeDialogueLine(entry.lines[j]);

                    if (!merged.empty())
                        merged += '\n';

                    merged += line;
                }

                char *copy = (char *)malloc(merged.size() + 1);
                strcpy(copy, merged.c_str());

                if (key.find("_h") != std::string::npos)
                    outHeader->push_back(copy);
                else
                    out->push_back(copy);
            }
        }

        sectionName = strtok(NULL, "-");
    }
}

inline const char *GetSplitLine(const char *text, int half)
{
    static char buffers[2][256];
    char *buffer = buffers[half & 1];

    if (!text)
    {
        strcpy(buffer, " ");
        return buffer;
    }

    const char *start = text;

    if (half)
    {
        start = strchr(text, '\n');
        if (!start)
        {
            strcpy(buffer, " ");
            return buffer;
        }
        ++start;
    }

    const char *end = strchr(start, '\n');
    size_t len = end ? (size_t)(end - start) : strlen(start);

    if (len == 0)
    {
        strcpy(buffer, " ");
        return buffer;
    }

    if (len >= 255)
        len = 255;

    memcpy(buffer, start, len);
    buffer[len] = '\0';

    return buffer;
}