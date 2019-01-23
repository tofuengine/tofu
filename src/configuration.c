#include "configuration.h"

#include "file.h"
#include "log.h"

#include <jsmn/jsmn.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JSON_TOKENS             32
#define MAX_JSON_KEY_LENGTH         32
#define MAX_JSON_VALUE_LENGTH       32

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240
#define WINDOW_TITLE    ".: TOFU :."

static void parse_pair(Configuration_t *configuration, const char *key, const char *value, int type)
{
    if (strcmp(key, "title") == 0) {
        strncpy(configuration->title, value, MAX_CONFIGURATION_TITLE_LENGTH);
    } else
    if (strcmp(key, "width") == 0) {
        configuration->width = (int)strtod(value, NULL);
    } else
    if (strcmp(key, "height") == 0) {
        configuration->height = (int)strtod(value, NULL);
    } else
    if (strcmp(key, "depth") == 0) {
        configuration->depth = (int)strtod(value, NULL);
    } else
    if (strcmp(key, "fullscreen") == 0) {
        configuration->fullscreen = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "autofit") == 0) {
        configuration->autofit = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "fps") == 0) {
        configuration->fps = (int)strtod(value, NULL);
    } else
    if (strcmp(key, "hide_cursor") == 0) {
        configuration->hide_cursor = strcmp(value, "true") == 0;
    } else
    if (strcmp(key, "debug") == 0) {
        configuration->debug = strcmp(value, "true") == 0;
    }
}

static void extract_token(const char *json, const jsmntok_t token, char *buffer, size_t buffer_size)
{
    size_t token_length = token.end - token.start;
    memcpy(buffer, json + token.start, token_length);
    buffer[token_length] = '\0';
}

void Configuration_initialize(Configuration_t *configuration)
{
    strncpy(configuration->title, WINDOW_TITLE, MAX_CONFIGURATION_TITLE_LENGTH);
    configuration->width = SCREEN_WIDTH;
    configuration->height = SCREEN_HEIGHT;
    configuration->depth = 8;
    configuration->fullscreen = false;
    configuration->autofit = true;
    configuration->fps = 60;
    configuration->hide_cursor = true;
    configuration->debug = true;
}

void Configuration_load(Configuration_t *configuration, const char *filename)
{
    char *json = file_load_as_string(filename, "rt");
    if (!json) {
        Log_write(LOG_LEVELS_WARNING, "Configuration file '%s' not found", filename);
        return;
    }

    jsmn_parser parser;
    jsmn_init(&parser);
    jsmntok_t tokens[MAX_JSON_TOKENS];
    size_t token_count = jsmn_parse(&parser, json, strlen(json), tokens, MAX_JSON_TOKENS);
    if ((token_count < 1) || (tokens[0].type != JSMN_OBJECT)) {
        Log_write(LOG_LEVELS_WARNING, "Configuration file '%s' is malformed", filename);
    }

    for (size_t i = 1; i < token_count; i += 2) {
        jsmntok_t json_key = tokens[i];
        if (json_key.type != JSMN_STRING) {
            Log_write(LOG_LEVELS_WARNING, "Configuration token #%d not of STRING type", i);
            break;
        }

        jsmntok_t json_value = tokens[i + 1];
#if 0
        if (json_value.type != JSMN_PRIMITIVE) {
            Log_write(LOG_LEVELS_WARNING, "Token #%d is not of PRIMITIVE type", i + 1);
            break;
        }
#endif

        char key[MAX_JSON_KEY_LENGTH], value[MAX_JSON_VALUE_LENGTH];
        extract_token(json, json_key, key, MAX_JSON_KEY_LENGTH);
        extract_token(json, json_value, value, MAX_JSON_VALUE_LENGTH);

        parse_pair(configuration, key, value, json_value.type);
    }

    free(json);
}
