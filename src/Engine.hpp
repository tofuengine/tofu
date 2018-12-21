#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <chaiscript/chaiscript.hpp>

#include "Display.hpp"

class Engine {

private:

    chaiscript::ChaiScript _interpreter;

    Display _display;

public:

    bool initialize();

    void terminate();

    void run();

};

#endif  /* __ENGINE_H__ */