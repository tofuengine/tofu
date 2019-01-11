#include "configuration.h"

#include <jsmn/jsmn.h>
#include <raylib/raylib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define countof(a)      (sizeof((a)) / sizeof(*(a)))

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define WINDOW_TITLE    ".: tOFu :."

bool Configuration_load(Configuration_t *configuration, const char *filename)
{
    strncpy(configuration->title, WINDOW_TITLE, MAX_CONFIGURATION_TITLE_LENGTH);
    configuration->width = SCREEN_WIDTH;
    configuration->height = SCREEN_HEIGHT;
    configuration->depth = 8;
    configuration->fullscreen = false;
    configuration->autofit = true;
    configuration->fps = 60;
    configuration->debug = true;

    FILE *file = fopen(filename, "rt");
    if (!file) {
        TraceLog(LOG_WARNING, "Configuration file '%s' not found", filename);
        return false;
    }
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    char json[size];
    fseek(file, 0L, SEEK_SET);
    fread(json, sizeof(char), size, file);
    fclose(file);

    jsmn_parser parser;
    jsmn_init(&parser);
    jsmntok_t tokens[32];
    int token_count = jsmn_parse(&parser, json, size, tokens, countof(tokens));
    if ((token_count < 0) || (tokens[0].type != JSMN_OBJECT)) {
        TraceLog(LOG_WARNING, "Configuration file '%s' is malformed", filename);
        return false;
    }

    return true;
}
