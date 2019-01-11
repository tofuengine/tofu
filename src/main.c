#include "engine.h"

#include <limits.h>
#include <stdlib.h>

#if 0
void chai_log(const std::string &message)
{
    raylib::TraceLog(raylib::LOG_DEBUG, message.c_str());
}

static void split_path(const std::string &path, std::string &parent, std::string &child)
{
#ifdef __linux__
    const char PATH_SEPARATOR_CHAR = '/';
    const char *PATH_SEPARATOR_STRING = "/";
#elif _WIN32
    const char PATH_SEPARATOR_CHAR = '\\';
    const char *PATH_SEPARATOR_STRING = "\\";
#endif
    std::string::size_type i = path.rfind(PATH_SEPARATOR_CHAR);
    if (i != std::string::npos) {
        parent = path.substr(0, i);
        child = path.substr(i + 1);
    } else {
        parent = "./";
        child = path;
    }

    char absolute_path[PATH_MAX];
    realpath(parent.c_str(), absolute_path);
    parent.assign(absolute_path);
    parent.append(PATH_SEPARATOR_STRING); // ChaiScript need the paths to end with the separator!
    free(absolute_path);

    raylib::TraceLog(raylib::LOG_DEBUG, "path '%s' for file '%s'", parent.c_str(), child.c_str());
}
#endif

int main(int argc, char **argv)
{
    const char *basepath = "./";
    if (argc > 1) {
        basepath = argv[1];
    }
    char absolute_path[PATH_MAX];
    realpath(basepath, absolute_path);

    Engine_t engine;
    Engine_initialize(&engine, absolute_path);
#if 0
    std::string script = "./boot.chai";
    if (argc > 1) {
        script = argv[1];
    }

    std::vector<std::string> modulepaths;
    std::vector<std::string> usepaths;
    std::string parent, child;
    split_path(script, parent, child);
    modulepaths.push_back(parent);
    usepaths.push_back(parent);

    chaiscript::ChaiScript chai(modulepaths, usepaths);
    chai.add(chaiscript::fun(&chai_log), "log");
    chai.eval_file(script);
#endif
    Engine_run(&engine);

    Engine_terminate(&engine);

    return EXIT_SUCCESS;
}