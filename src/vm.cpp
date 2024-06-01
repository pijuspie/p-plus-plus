#include <iostream>
#include "vm.h"
#include "compiler.h"

VM global;

InterpretResult interpret(std::string& source) {
    return global.interpret(source);
}

void VM::runtimeError(const std::string& message) {
    std::cerr << message << std::endl;

    int instruction = ip - fn->chunk.code.begin() - 1;
    int line = fn->chunk.lines[instruction];
    std::cerr << "[line " << line << "] in script" << std::endl;
    stack.clear();
}

void VM::freeObjects() {
    while (objects != nullptr) {
        Obj* next = objects->next;

        switch (objects->type) {
        case VAL_STRING: delete (ObjString*)objects; break;
        case VAL_FUNCTION: delete (Function*)objects; break;
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
    return fn->chunk.constants[readByte()];
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case VAL_BOOL: return a.as.boolean == b.as.boolean;
    case VAL_NIL: return true;
    case VAL_NUMBER: return a.as.number == b.as.number;
    case VAL_STRING: return getString(a) == getString(b);
    case VAL_FUNCTION: return getFunction(a).name == getFunction(b).name;
    default: return false;
    }
}

bool isFalsey(Value value) {
    return value.type == VAL_NIL || (value.type == VAL_BOOL && !value.as.boolean);
}

InterpretResult VM::run() {
    for (;;) {
        uint8_t instruction = readByte();
        //std::cout << (int)instruction << std::endl;
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
            if (peek(0).type != VAL_NUMBER || peek(1).type != VAL_NUMBER) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().as.number < pop().as.number);
            break;
        }
        case OP_LESS: {
            if (peek(0).type != VAL_NUMBER || peek(1).type != VAL_NUMBER) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().as.number > pop().as.number);
            break;
        }
        case OP_NEGATE:
            if (peek(0).type != VAL_NUMBER) {
                runtimeError("Operand must be a number.");
                return InterpretResult::runtimeError;
            }
            push(-pop().as.number);
            break;
        case OP_ADD:
            if (peek(0).type == VAL_STRING && peek(1).type == VAL_STRING) {
                std::string b = getString(pop());
                std::string a = getString(pop());
                ObjString* result = new ObjString(a + b, objects);
                objects = (Obj*)result;
                push(Value(*result));
            } else if (peek(0).type == VAL_NUMBER && peek(1).type == VAL_NUMBER) {
                push(pop().as.number + pop().as.number);
            } else {
                std::cout << peek(0).type << " " << peek(1).type << std::endl;
                runtimeError("Operands must be two numbers or two strings.");
                return InterpretResult::runtimeError;
            }
            break;
        case OP_SUBTRACT: {
            if (peek(0).type != VAL_NUMBER || peek(1).type != VAL_NUMBER) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push((pop().as.number - pop().as.number) * -1);
            break;
        }
        case OP_MULTIPLY: {
            if (peek(0).type != VAL_NUMBER || peek(1).type != VAL_NUMBER) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().as.number * pop().as.number);
            break;
        }
        case OP_DIVIDE: {
            if (peek(0).type != VAL_NUMBER || peek(1).type != VAL_NUMBER) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(1 / pop().as.number * pop().as.number);
            break;
        }
        case OP_NOT: {
            Value value = pop();
            push(isFalsey(value));
            break;
        }
        case OP_PRINT: {
            std::cout << stringify(pop()) << std::endl;
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
            std::string name = getString(readConstant());
            std::unordered_map<std::string, Value>::iterator value = globals.find(name);

            if (value != globals.end()) {
                value->second = peek(0);
            } else {
                globals.insert({ name, peek(0) });
            }

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
            std::string name = getString(readConstant());
            std::unordered_map<std::string, Value>::iterator value = globals.find(name);

            if (value == globals.end()) {
                runtimeError("Undefined variable '" + name + "'.");
                return InterpretResult::runtimeError;
            }

            push(value->second);
            break;
        }
        case OP_SET_GLOBAL: {
            std::string name = getString(readConstant());
            std::unordered_map<std::string, Value>::iterator value = globals.find(name);

            if (value == globals.end()) {
                runtimeError("Undefined variable '" + name + "'.");
                return InterpretResult::runtimeError;
            }

            value->second = peek(0);
            break;
        }
        }
    }
}

InterpretResult VM::interpret(std::string& source) {
    fn = compile(source, objects);

    if (fn == nullptr) {
        return InterpretResult::compileError;
    }

    stack.push_back(Value(*fn));
    ip = fn->chunk.code.begin();

    InterpretResult result = run();
    stack.pop_back();

    //freeObjects();
    return result;
}