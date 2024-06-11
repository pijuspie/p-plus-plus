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
    Upvalue* openUpvalues = nullptr;
    Object* objects = nullptr;

    bool clockNative(int argCount, Value* args);
    bool readNumberNative(int argCount, Value* args);

    void runtimeError(const std::string& format);
    void defineNative(std::string name, NativeFn function);

    void push(Value value);
    Value pop();
    Value peek(int distance);
    bool call(Closure* closure, int argCount);
    bool callValue(Value callee, int argCount);
    Upvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);

    uint8_t readByte();
    uint16_t readShort();
    Value readConstant();
    InterpretResult run();
public:
    InterpretResult interpret(std::string& source);
    VM();
};

InterpretResult interpret(std::string& source);

#endif