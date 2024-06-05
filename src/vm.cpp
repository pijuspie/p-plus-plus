#include <iostream>
#include "vm.h"
#include "compiler.h"

VM global;

CallFrame::CallFrame(Function* function1, int slots1) {
    function = function1;
    ip = function->chunk.code.begin();
    slots = slots1;
}

InterpretResult interpret(std::string& source) {
    return global.interpret(source);
}

void VM::runtimeError(const std::string& message) {
    std::cerr << message << std::endl;

    for (int i = frames.size() - 1; i >= 0; i--) {
        CallFrame* frame = &frames[i];
        Function& function = *frame->function;

        size_t instruction = frame->ip - function.chunk.code.begin() - 1;
        std::cerr << "[line " << function.chunk.lines[instruction] << "] in ";
        if (function.name == "") {
            std::cerr << "script" << std::endl;
        } else {
            std::cerr << function.name << "()" << std::endl;
        }
        frames.pop_back();
    }

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

bool VM::call(Function& function, int argCount) {
    if (argCount != function.arity) {
        runtimeError("Expected " + std::to_string(function.arity) + " arguments but got " + std::to_string(argCount) + ".");
        return false;
    }

    frames.push_back(CallFrame(&function, stack.size() - argCount - 1));
    return true;
}

bool VM::callValue(Value callee, int argCount) {
    if (callee.type == VAL_FUNCTION) {
        return call(getFunction(callee), argCount);
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

uint8_t VM::readByte() {
    CallFrame& frame = frames.back();
    frame.ip++;
    return frame.ip[-1];
}

uint16_t VM::readShort() {
    CallFrame& frame = frames.back();
    frame.ip += 2;
    return (uint16_t)((frame.ip[-2] << 8) | frame.ip[-1]);
}

Value VM::readConstant() {
    CallFrame& frame = frames.back();
    return frame.function->chunk.constants[readByte()];
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
    CallFrame* frame = &frames[frames.size() - 1];

    for (;;) {
        uint8_t instruction = readByte();
        //std::cout << getOpCode(OpCode(instruction)) << std::endl;
        switch (instruction) {
        case OP_CALL: {
            int argCount = readByte();
            if (!callValue(peek(argCount), argCount)) {
                return InterpretResult::runtimeError;
            }

            frame = &frames[frames.size() - 1];
            break;
        }
        case OP_RETURN: {
            Value result = pop();
            int slots = frame->slots;

            frames.pop_back();
            if (frames.size() == 0) {
                pop();
                return InterpretResult::ok;
            }

            stack.resize(slots);
            push(result);
            frame = &frames[frames.size() - 1];
            break;
        }
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
            frame->ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = readShort();
            if (isFalsey(peek(0))) frame->ip += offset;
            break;
        }
        case OP_LOOP: {
            frame->ip -= readShort();
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
            push(stack[frame->slots + readByte()]);
            break;
        case OP_SET_LOCAL: {
            uint8_t slot = readByte();
            stack[frame->slots + slot] = peek(0);
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
    Function* fn = compile(source, objects);

    if (fn == nullptr) {
        return InterpretResult::compileError;
    }

    stack.push_back(Value(*fn));
    call(*fn, 0);

    InterpretResult result = run();

    //freeObjects();
    return result;
}