#ifndef object_h
#define object_h

#include <string>
#include "value.h"
#include "chunk.h"

struct Obj {
    ObjType type;
    Obj* next;
    Obj(ObjType type, Obj* next);
};

struct ObjString {
    Obj obj;
    std::string string;
    ObjString(const std::string& string, Obj* next);
};

struct ObjFunction {
    Obj obj;
    int arity = 0;
    Chunk chunk;
    std::string name;

    ObjFunction(Obj* next);
};

#endif