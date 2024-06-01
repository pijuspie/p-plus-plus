#ifndef compiler_h
#define compiler_h

#include "chunk.h"
#include "scanner.h"
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
    ObjFunction* function = nullptr;
    FunctionType type = TYPE_SCRIPT;

    std::vector<Local> locals;
    int scopeDepth = 0;
};

ObjFunction* compile(const std::string& source, Obj* objects);

#endif 