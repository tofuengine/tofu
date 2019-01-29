#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

#define UNUSED(x)   (void)(x)

void file_resolve_path(char *resolved, const char *path)
{
    char *ptr = realpath(path, resolved);
    UNUSED(ptr);
    size_t length = strlen(resolved);
    if (resolved[length - 1] != '/') {
        strcat(resolved, "/");
    }
}

char *file_load_as_string(const char *filename, const char *mode)
{
    FILE *file = fopen(filename, mode);
    if (!file) {
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    size_t length = ftell(file);
    char *data = Memory_calloc(length + 1, sizeof(char)); // Add null terminator for the string.
    rewind(file);
    size_t read_bytes = fread(data, sizeof(char), length, file);
    fclose(file);
    if (read_bytes < length) {
        free(data);
        return NULL;
    }
    data[length] = '\0';
    return data;
}