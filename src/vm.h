#ifndef vm_h
#define vm_h

#include <vector>
#include <unordered_map>
#include "value.h"

enum class InterpretResult {
    ok, compileError, runtimeError
};

class VM {
private:
    Function* fn;
    std::vector<uint8_t>::iterator ip;
    std::vector<Value> stack;
    std::unordered_map<std::string, Value> globals;

    Obj* objects = nullptr;

    void runtimeError(const std::string& format);
    void freeObjects();

    void push(Value value);
    Value pop();
    Value peek(int distance);

    uint8_t readByte();
    uint16_t readShort();
    Value readConstant();
    InterpretResult run();
public:
    InterpretResult interpret(std::string& source);
};

InterpretResult interpret(std::string& source);

#endif