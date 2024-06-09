#ifndef compiler_h
#define compiler_h

#include "scanner.h"
#include "value.h"
#include <string>

struct Local {
    Token name;
    bool isCaptured = false;
    int depth;

    Local(Token name, int depth);
};

struct Upvalue {
    uint8_t index;
    bool isLocal;
    Upvalue(bool isLocal, uint8_t index);
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
    std::vector<Upvalue> upvalues;
    int scopeDepth = 0;

    Compiler(Compiler* enclosing, FunctionType type, Obj* objects);
};

Function* compile(const std::string& source, Obj* objects);

#endif 