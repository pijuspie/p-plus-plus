#ifndef memory_h
#define memory_h

#include "value.h"
#include "scanner.h"
#include <vector>
#include <unordered_map>

typedef struct GC GC;

struct CallFrame {
    Closure* closure;
    std::vector<uint8_t>::iterator ip;
    int slots;

    CallFrame(Closure* closure, int slots);
};

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
    GC* garbageCollector;

    std::vector<Local> locals;
    std::vector<OpenUpvalue> upvalues;
    int scopeDepth = 0;

    Compiler(Compiler* enclosing, Token name, FunctionType type, GC* garbageCollector);
};

struct GC {
    size_t bytesAllocated = 0;
    size_t nextGC = 50; //1024 * 1024;
    Object* objects = nullptr;
    std::vector<Object*> grayObjects;

    std::vector<Value>* stack;
    Upvalue** openUpvalues;
    std::unordered_map<std::string, Value>* globals;
    std::vector<CallFrame>* frames;
    Compiler* compiler = nullptr;

    void markObject(Object* object);
    void markValue(Value value);
    void markRoots();
    void blackenObject(Object* object);
    void traceReferences();
    void sweep();
    void collectGarbage();

    String* newString(std::string& chars);
    Function* newFunction(std::string& name);
    Native* newNative(NativeFn function);
    Upvalue* newUpvalue(Value* location, Upvalue* next);
    Closure* newClosure(Function* function);
    void freeObject(Object* object);
    void freeObjects();
};


#endif