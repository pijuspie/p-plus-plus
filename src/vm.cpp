#include <iostream>
#include "vm.h"
#include "compiler.h"

VM global;

InterpretResult interpret(std::string& source) {
    return global.interpret(source);
}

void VM::runtimeError(const std::string& message) {
    std::cerr << message << std::endl;

    int instruction = ip - chunk.code.begin() - 1;
    int line = chunk.lines[instruction];
    std::cerr << "[line " << line << "] in script" << std::endl;
    stack.clear();
}

void VM::freeObjects() {
    while (objects != nullptr) {
        Obj* next = objects->next;

        switch (objects->type) {
        case OBJ_STRING: delete (ObjString*)objects; break;
        }

        objects = next;
    }
}

void VM::push(Value value) {
    stack.push_back(value);
}

Value VM::pop() {
    Value x = stack[stack.size() - 1];
    stack.pop_back();
    return x;
}

Value VM::peek(int distance) {
    return stack[stack.size() - 1 - distance];
}

uint8_t VM::readByte() {
    ip++;
    return ip[-1];
}

uint16_t VM::readShort() {
    ip += 2;
    return (uint16_t)((ip[-2] << 8) | ip[-1]);
}

Value VM::readConstant() {
    return chunk.constants[readByte()];
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case VAL_BOOL: return a.getBool() == b.getBool();
    case VAL_NIL: return true;
    case VAL_NUMBER: return a.getNumber() == b.getNumber();
    case VAL_OBJ: return a.getString() == b.getString();
    default: return false;
    }
}

bool isFalsey(Value value) {
    return value.isNil() || (value.isBool() && !value.getBool());
}

InterpretResult VM::run() {
    for (;;) {
        uint8_t instruction = readByte();
        // std::cout << (int)instruction << std::endl;
        switch (instruction) {
        case OP_RETURN:
            return InterpretResult::ok;
        case OP_CONSTANT: {
            Value constant = readConstant();
            push(constant);
            break;
        }
        case OP_NIL: push(Value()); break;
        case OP_TRUE: push(true); break;
        case OP_FALSE: push(false); break;
        case OP_EQUAL: push(valuesEqual(pop(), pop())); break;
        case OP_GREATER: {
            if (!peek(0).isNumber() || !peek(1).isNumber()) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().getNumber() < pop().getNumber());
            break;
        }
        case OP_LESS: {
            if (!peek(0).isNumber() || !peek(1).isNumber()) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().getNumber() > pop().getNumber());
            break;
        }
        case OP_NEGATE:
            if (!peek(0).isNumber()) {
                runtimeError("Operand must be a number.");
                return InterpretResult::runtimeError;
            }
            push(-pop().getNumber());
            break;
        case OP_ADD:
            if (peek(0).isString() && peek(1).isString()) {
                ObjString* b = pop().getObjString();
                ObjString* a = pop().getObjString();
                ObjString* result = new ObjString(a->string + b->string, objects);
                objects = (Obj*)result;
                push((Obj*)result);
            } else if (peek(0).isNumber() && peek(1).isNumber()) {
                push(pop().getNumber() + pop().getNumber());
            } else {
                runtimeError("Operands must be two numbers or two strings.");
                return InterpretResult::runtimeError;
            }
            break;
        case OP_SUBTRACT: {
            if (!peek(0).isNumber() || !peek(1).isNumber()) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push((pop().getNumber() - pop().getNumber()) * -1);
            break;
        }
        case OP_MULTIPLY: {
            if (!peek(0).isNumber() || !peek(1).isNumber()) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().getNumber() * pop().getNumber());
            break;
        }
        case OP_DIVIDE: {
            if (!peek(0).isNumber() || !peek(1).isNumber()) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(1 / pop().getNumber() * pop().getNumber());
            break;
        }
        case OP_NOT: {
            Value value = pop();
            push(value.isNil() || (value.isBool() && !value.getBool()));
            break;
        }
        case OP_PRINT: {
            pop().print();
            std::cout << std::endl;
            break;
        }
        case OP_JUMP: {
            uint16_t offset = readShort();
            ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = readShort();
            if (isFalsey(peek(0))) ip += offset;
            break;
        }
        case OP_LOOP: {
            ip -= readShort();
            break;
        }
        case OP_POP: pop(); break;
        case OP_DEFINE_GLOBAL: {
            globals.insert({ readConstant().getObjString()->string, peek(0) });
            pop();
            break;
        }
        case OP_GET_LOCAL:
            push(stack[readByte()]);
            break;
        case OP_SET_LOCAL: {
            uint8_t slot = readByte();
            stack[slot] = peek(0);
            break;
        }
        case OP_GET_GLOBAL: {
            ObjString* name = readConstant().getObjString();
            std::unordered_map<std::string, Value>::iterator value = globals.find(name->string);

            if (value == globals.end()) {
                runtimeError("Undefined variable '" + name->string + "'.");
                return InterpretResult::runtimeError;
            }

            push(value->second);
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString* name = readConstant().getObjString();
            std::unordered_map<std::string, Value>::iterator value = globals.find(name->string);

            if (value == globals.end()) {
                runtimeError("Undefined variable '" + name->string + "'.");
                return InterpretResult::runtimeError;
            }

            value->second = peek(0);
            break;
        }
        }
    }
}

InterpretResult VM::interpret(std::string& source) {
    chunk = Chunk();

    Compiler compiler;
    if (!compile(source, chunk, objects, compiler)) {
        return InterpretResult::compileError;
    }

    ip = chunk.code.begin();
    InterpretResult result = run();

    //freeObjects();
    return result;
}