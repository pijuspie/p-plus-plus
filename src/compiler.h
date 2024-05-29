#ifndef compiler_h
#define compiler_h

#include <string>
#include "chunk.h"
#include "scanner.h"

struct Local {
    Token name;
    int depth;
};

struct Compiler {
    std::vector<Local> locals;
    int scopeDepth = 0;
};

bool compile(const std::string& source, Chunk& chunk, Obj* objects, Compiler& compiler);

#endif 