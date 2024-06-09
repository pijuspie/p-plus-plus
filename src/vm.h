#ifndef vm_h
#define vm_h

#include <vector>
#include <unordered_map>
#include "value.h"

enum class InterpretResult {
    ok, compileError, runtimeError
};

struct CallFrame {
    Closure* closure;
    std::vector<uint8_t>::iterator ip;
    int slots;

    CallFrame(Closure* closure, int slots);
};

class VM {
private:
    std::vector<CallFrame> frames;
    std::vector<Value> stack;
    std::unordered_map<std::string, Value> globals;

    Obj* objects = nullptr;

    bool clockNative(int argCount, Value* args);
    bool readNumberNative(int argCount, Value* args);

    void runtimeError(const std::string& format);
    void defineNative(const std::string& name, NativeFn function, Obj* objects);
    void freeObjects();

    void push(Value value);
    Value pop();
    Value peek(int distance);
    bool call(Closure& closure, int argCount);
    bool callValue(Value callee, int argCount);
    ObjUpvalue* captureUpvalue(Value* local);

    uint8_t readByte();
    uint16_t readShort();
    Value readConstant();
    InterpretResult run();
public:
    InterpretResult interpret(std::string& source);
};

InterpretResult interpret(std::string& source);

#endif