/*
 * MIT License
 * 
 * Copyright (c) 2019-2023 Marco Lizza
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <core/config.h>
#include <core/engine.h>
#include <core/platform.h>
#include <libs/path.h>

#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#define LOG_CONTEXT "main"

static struct option _long_options[] = {
    { "help", no_argument, NULL, 'h' },
    { "kernal", optional_argument, NULL, 'k' },
    { "data", optional_argument, NULL, 'd' },
    { NULL, 0, NULL, 0 }
};

static void _print_usage(int argc, const char *argv[])
{
    fprintf(stderr, "Usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-h, --help\t\tShow this help.\n");
    fprintf(stderr, "\t-k, --kernal <path>\tPath of the folder or file to mount (can be repeated).\n");
    fprintf(stderr, "\t-d, --data <path>\tPath of the folder or file to mount (can be repeated).\n");
}

static void _options_init(Engine_Options_t *options, const char *argv0)
{
    char executable[PLATFORM_PATH_MAX] = { 0 }; // From the executable path obtain the kernal file path.
    path_expand(argv0, executable);

    char executable_path[PLATFORM_PATH_MAX] = { 0 };
    path_split(executable, executable_path, NULL);

    path_join(options->kernal_path, executable_path, TOFU_ENGINE_KERNAL_NAME);
    path_join(options->data_path, executable_path, TOFU_ENGINE_DATA_NAME);
}

static bool _parse_command_line(int argc, const char *argv[], Engine_Options_t *options)
{
    *options = (Engine_Options_t){ 0 };

    _options_init(options, argv[0]);

    while (true) {
        int option = getopt_long(argc, (char * const *)argv, "hk:d:", _long_options, NULL);
        if (option == -1) {
            break;
        }
        switch (option) {
            case 'h': {
                _print_usage(argc, argv);
                return false;
            }
            case 'k': {
                strncpy(options->kernal_path, optarg, PLATFORM_PATH_MAX - 1);
                break;
            }
            case 'd': {
                strncpy(options->data_path, optarg, PLATFORM_PATH_MAX - 1);
                break;
            }
            default: {
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return false;
            }
        }
    }

    return true;
}

int main(int argc, const char *argv[])
{
    Engine_Options_t options = { 0 };
    bool parsed = _parse_command_line(argc, argv, &options);
    if (!parsed) {
        return EXIT_FAILURE;
    }

    Engine_t *engine = Engine_create(&options);
    if (!engine) {
        return EXIT_FAILURE;
    }
    Engine_run(engine);
    Engine_destroy(engine);

    return EXIT_SUCCESS;
}
