#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void resolve_path(const char *path, char *resolved)
{
    realpath(path, resolved);
    size_t length = strlen(resolved);
    if (resolved[length - 1] != '/') {
        strcat(resolved, "/");
    }
}

char *load_file_as_string(const char *filename, const char *mode)
{
    FILE *file = fopen(filename, mode);
    if (!file) {
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    size_t length = ftell(file);
    char *data = calloc(length + 1, sizeof(char)); // Add null terminator for the string.
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