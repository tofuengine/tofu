#include "engine.h"

#include "log.h"
#include "utils.h"

#include <limits.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const char *basepath = "./";
    if (argc > 1) {
        basepath = argv[1];
    }
    char resolved[PATH_MAX];
    resolve_path(basepath, resolved);

    Engine_t engine;
    Engine_initialize(&engine, resolved);

    Engine_run(&engine);

    Engine_terminate(&engine);

    return EXIT_SUCCESS;
}