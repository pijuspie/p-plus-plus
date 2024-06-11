#ifndef memory_h
#define memory_h

#include "value.h"

struct GC {
    size_t bytesAllocated = 0;
    size_t nextGC = 1024 * 1024;
    Object* objects = nullptr;

    void collectGarbage();
    String* newString(std::string& chars);
    Function* newFunction(std::string& name);
    Native* newNative(NativeFn function);
    Upvalue* newUpvalue(Value* location, Upvalue* next);
    Closure* newClosure(Function* function);
    void freeObjects();
};


#endif