#include "common.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>

Entry entries[MAX_FILES];
int entryCount = 0, selected = 0, scrollOffset = 0;
char currentDir[MAX_PATH] = "sdmc:/Music";

Entry playlist[MAX_FILES];
int playlistCount = 0;

static int entrySort(const void *a, const void *b) {
    const Entry *ea = (const Entry *)a, *eb = (const Entry *)b;
    if (ea->type != eb->type) return ea->type == ENTRY_DIR ? -1 : 1;
    return strcasecmp(ea->name, eb->name);
}

void scanDir(const char *path) {
    entryCount = selected = scrollOffset = 0;
    DIR *dir = opendir(path);
    if (!dir) return;
    struct dirent *e;
    while ((e = readdir(dir)) && entryCount < MAX_FILES) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        Entry en;
        strncpy(en.name, e->d_name, 255); en.name[255] = '\0';
        snprintf(en.fullPath, MAX_PATH, "%s/%s", path, e->d_name);
        if (e->d_type == DT_DIR) {
            en.type = ENTRY_DIR;
            entries[entryCount++] = en;
        } else {
            size_t l = strlen(e->d_name);
            if (l > 4 && !strcasecmp(e->d_name + l - 4, ".mp3")) {
                en.type = ENTRY_MP3;
                entries[entryCount++] = en;
            }
        }
    }
    closedir(dir);
    qsort(entries, entryCount, sizeof(Entry), entrySort);
}

bool goUp(void) {
    if (!strcmp(currentDir, "sdmc:/") || !strcmp(currentDir, "sdmc:")) return false;
    char *slash = strrchr(currentDir, '/');
    if (!slash) return false;
    if (slash == currentDir + 6) *(slash + 1) = '\0';
    else *slash = '\0';
    return true;
}
