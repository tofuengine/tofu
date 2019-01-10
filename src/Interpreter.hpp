#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <chaiscript/chaiscript.hpp>

class Interpreter {

private:

    chaiscript::ChaiScript _chai;

public:

    bool initialize();

    void terminate();

};

#endif  /* __INTERPRETER_H__ */