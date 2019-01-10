#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "Display.hpp"
#include "Interpreter.hpp"

class Engine {

private:

    Display _display;
    Interpreter _interpreter;

public:

    bool initialize();

    void terminate();

    void run();

};

#endif  /* __ENGINE_H__ */