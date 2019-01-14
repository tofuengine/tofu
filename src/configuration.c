#include "configuration.h"

#include "log.h"
#include "utils.h"

#include <jsmn/jsmn.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define countof(a)      (sizeof((a)) / sizeof(*(a)))

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define WINDOW_TITLE    ".: tOFu :."

void Configuration_load(Configuration_t *configuration, const char *filename)
{
    strncpy(configuration->title, WINDOW_TITLE, MAX_CONFIGURATION_TITLE_LENGTH);
    configuration->width = SCREEN_WIDTH;
    configuration->height = SCREEN_HEIGHT;
    configuration->depth = 8;
    configuration->fullscreen = false;
    configuration->autofit = true;
    configuration->fps = 60;
    configuration->debug = true;

    char *json = load_file_as_string(filename, "rt");
    if (!json) {
        Log_write(LOG_LEVELS_WARNING, "Configuration file '%s' not found", filename);
        return;
    }

    jsmn_parser parser;
    jsmn_init(&parser);
    jsmntok_t tokens[32];
    int token_count = jsmn_parse(&parser, json, strlen(json), tokens, countof(tokens));
    if ((token_count < 0) || (tokens[0].type != JSMN_OBJECT)) {
        Log_write(LOG_LEVELS_WARNING, "Configuration file '%s' is malformed", filename);
    }

    free(json);
}
