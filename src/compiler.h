#ifndef compiler_h
#define compiler_h

#include "scanner.h"
#include "value.h"
#include <string>

struct Local {
    Token name;
    int depth;

    Local(Token name, int depth);
};

enum FunctionType {
    TYPE_FUNCTION,
    TYPE_SCRIPT
};

struct Compiler {
    Compiler* enclosing;
    Function* function;
    FunctionType type;

    std::vector<Local> locals;
    int scopeDepth = 0;

    Compiler(Compiler* enclosing, FunctionType type, Obj* objects);
};

Function* compile(const std::string& source, Obj* objects);

#endif 