#ifndef vm_h
#define vm_h

#include <vector>
#include <unordered_map>
#include "value.h"
#include "memory.h"

enum class InterpretResult {
    ok, compileError, runtimeError
};

class VM {
private:
    std::vector<CallFrame> frames;
    std::vector<Value> stack;
    std::unordered_map<std::string, Value> globals;
    String* initString = nullptr;
    Upvalue* openUpvalues = nullptr;
    GC garbageCollector;

    bool clockNative(int argCount, Value* args);
    bool readNumberNative(int argCount, Value* args);
    bool stringifyNative(int argCount, Value* args);
    bool roundNative(int argCount, Value* args);

    void runtimeError(const std::string& format);
    void defineNative(std::string name, NativeFn function);

    void push(Value value);
    Value pop();
    Value peek(int distance);
    bool call(Closure* closure, int argCount);
    bool callValue(Value callee, int argCount);
    bool invokeFromClass(Class* klass, String* name, int argCount);
    bool invoke(String* name, int argCount);
    bool bindMethod(Class* klass, String* name);
    Upvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);
    void defineMethod(String* name);

    uint8_t readByte();
    uint16_t readShort();
    Value readConstant();
    InterpretResult run();
public:
    VM();
    InterpretResult interpret(std::string& source);
    ~VM();
};

InterpretResult interpret(std::string& source);

#endif