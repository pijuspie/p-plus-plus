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

struct OpenUpvalue {
    uint8_t index;
    bool isLocal;
    OpenUpvalue(bool isLocal, uint8_t index);
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
    std::vector<OpenUpvalue> upvalues;
    int scopeDepth = 0;

    Compiler(Compiler* enclosing, FunctionType type, Object*& objects);
};

Function* compile(const std::string& source, Object*& objects);

#endif 