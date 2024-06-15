#include <iostream>
#include <time.h>
#include "vm.h"
#include "compiler.h"

VM global;

CallFrame::CallFrame(Closure* closure1, int slots1) {
    closure = closure1;
    ip = closure->function->chunk.code.begin();
    slots = slots1;
}

InterpretResult interpret(std::string& source) {
    return global.interpret(source);
}

VM::VM() {
    garbageCollector.stack = &stack;
    garbageCollector.globals = &globals;
    garbageCollector.frames = &frames;
    garbageCollector.openUpvalues = &openUpvalues;

    stack.reserve(256);
    defineNative("clock", clockNative);
    defineNative("readNumber", readNumberNative);
    defineNative("stringify", stringifyNative);
}

bool VM::clockNative(int argCount, Value* args) {
    if (argCount > 0) {
        runtimeError("Expected 0 arguments but got " + std::to_string(argCount) + ".");
        return false;
    }

    push(Value((double)clock() / CLOCKS_PER_SEC));
    return true;
}

bool VM::readNumberNative(int argCount, Value* args) {
    if (argCount > 0) {
        runtimeError("Expected 0 arguments but got " + std::to_string(argCount) + ".");
        return false;
    }

    std::string x;
    std::getline(std::cin, x);
    double d;

    try {
        d = std::stod(x);
    } catch (const std::invalid_argument&) {
        d = 0;
    }

    push(Value(d));
    return true;
}

bool VM::stringifyNative(int argCount, Value* args) {
    if (argCount != 1) {
        runtimeError("Expected 1 arguments but got " + std::to_string(argCount) + ".");
        return false;
    }

    std::string chars = args[0].stringify();
    String* string = garbageCollector.newString(chars);
    push(Value(string));
    return true;
}

void VM::runtimeError(const std::string& message) {
    std::cerr << message << std::endl;

    for (int i = frames.size() - 1; i >= 0; i--) {
        CallFrame* frame = &frames[i];
        Function& function = *frame->closure->function;

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

void VM::defineNative(std::string name, NativeFn function) {
    Native* native = garbageCollector.newNative(function);
    push(Value(native));

    globals.insert({ name, stack[0] });
    pop();
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

bool VM::call(Closure* closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected " + std::to_string(closure->function->arity) + " arguments but got " + std::to_string(argCount) + ".");
        return false;
    }

    frames.push_back(CallFrame(closure, stack.size() - argCount - 1));
    return true;
}

bool VM::callValue(Value callee, int argCount) {
    if (callee.type != ValueType::object) {
        runtimeError("Can only call functions and classes.");
        return false;
    }

    switch (callee.as.object->type) {
    case ObjectType::native: {
        NativeFn native = callee.getNative()->function;
        if (!(this->*native)(argCount, &stack[stack.size() - argCount])) {
            return false;
        }
        Value result = pop();
        stack.resize(stack.size() - argCount - 1);
        push(result);
        return true;
    }
    case ObjectType::closure:
        return call(callee.getClosure(), argCount);
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

Upvalue* VM::captureUpvalue(Value* local) {
    Upvalue* prevUpvalue = nullptr;
    Upvalue* upvalue = openUpvalues;
    while (upvalue != nullptr && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != nullptr && upvalue->location == local) {
        return upvalue;
    }

    Upvalue* createdUpvalue = garbageCollector.newUpvalue(local, upvalue);

    if (prevUpvalue == nullptr) {
        openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

void VM::closeUpvalues(Value* last) {
    while (openUpvalues != nullptr && openUpvalues->location >= last) {
        Upvalue* upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
    }
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
    return frame.closure->function->chunk.constants[readByte()];
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case ValueType::boolean: return a.as.boolean == b.as.boolean;
    case ValueType::nil: return true;
    case ValueType::number: return a.as.number == b.as.number;
    case ValueType::object: {
        switch (a.as.object->type) {
        case ObjectType::string: return a.getString()->chars == b.getString()->chars;
        case ObjectType::function: return a.getFunction()->name == b.getFunction()->name;
        default: return false;
        }
    }
    default: return false;
    }
}

bool isFalsey(Value value) {
    return value.type == ValueType::nil || (value.type == ValueType::boolean && !value.as.boolean);
}

InterpretResult VM::run() {
    CallFrame* frame = &frames[frames.size() - 1];

    for (;;) {
        uint8_t instruction = readByte();
        //std::cout << stringifyOpCode(OpCode(instruction)) << std::endl;
        switch (instruction) {
        case OP_CALL: {
            int argCount = readByte();
            if (!callValue(peek(argCount), argCount)) {
                return InterpretResult::runtimeError;
            }

            frame = &frames[frames.size() - 1];
            break;
        }
        case OP_CLOSURE: {
            Function* function = readConstant().getFunction();
            Closure* closure = garbageCollector.newClosure(function);
            push(Value(closure));
            for (int i = 0; i < function->upvalueCount; i++) {
                uint8_t isLocal = readByte();
                uint8_t index = readByte();
                if (isLocal) {
                    closure->upvalues.push_back(captureUpvalue(&stack[frame->slots + index]));
                } else {
                    closure->upvalues.push_back(frame->closure->upvalues[index]);
                }
            }
            break;
        }
        case OP_CLOSE_UPVALUE:
            closeUpvalues(&stack[stack.size() - 1]);
            pop();
            break;
        case OP_RETURN: {
            Value result = pop();
            int slots = frame->slots;
            closeUpvalues(&stack[slots]);

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
            if (peek(0).type != ValueType::number || peek(1).type != ValueType::number) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().as.number < pop().as.number);
            break;
        }
        case OP_LESS: {
            if (peek(0).type != ValueType::number || peek(1).type != ValueType::number) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().as.number > pop().as.number);
            break;
        }
        case OP_NEGATE:
            if (peek(0).type != ValueType::number) {
                runtimeError("Operand must be a number.");
                return InterpretResult::runtimeError;
            }
            push(-pop().as.number);
            break;
        case OP_ADD:
            if (peek(0).type == ValueType::object && peek(0).as.object->type == ObjectType::string &&
                peek(1).type == ValueType::object && peek(1).as.object->type == ObjectType::string) {
                std::string& b = pop().getString()->chars;
                std::string& a = pop().getString()->chars;
                std::string c = a + b;
                String* result = garbageCollector.newString(c);
                push(Value(result));
            } else if (peek(0).type == ValueType::number && peek(1).type == ValueType::number) {
                push(pop().as.number + pop().as.number);
            } else {
                runtimeError("Operands must be two numbers or two strings.");
                return InterpretResult::runtimeError;
            }
            break;
        case OP_SUBTRACT: {
            if (peek(0).type != ValueType::number || peek(1).type != ValueType::number) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push((pop().as.number - pop().as.number) * -1);
            break;
        }
        case OP_MULTIPLY: {
            if (peek(0).type != ValueType::number || peek(1).type != ValueType::number) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            push(pop().as.number * pop().as.number);
            break;
        }
        case OP_DIVIDE: {
            if (peek(0).type != ValueType::number || peek(1).type != ValueType::number) {
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
            std::cout << pop().stringify();
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
            std::string& name = readConstant().getString()->chars;
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
            std::string& name = readConstant().getString()->chars;
            std::unordered_map<std::string, Value>::iterator value = globals.find(name);

            if (value == globals.end()) {
                runtimeError("Undefined variable '" + name + "'.");
                return InterpretResult::runtimeError;
            }

            push(value->second);
            break;
        }
        case OP_SET_GLOBAL: {
            std::string& name = readConstant().getString()->chars;
            std::unordered_map<std::string, Value>::iterator value = globals.find(name);

            if (value == globals.end()) {
                runtimeError("Undefined variable '" + name + "'.");
                return InterpretResult::runtimeError;
            }

            value->second = peek(0);
            break;
        }
        case OP_GET_UPVALUE: {
            uint8_t slot = readByte();
            push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE: {
            uint8_t slot = readByte();
            *frame->closure->upvalues[slot]->location = peek(0);
            break;
        }
        }
    }
}

InterpretResult VM::interpret(std::string& source) {
    Function* fn = compile(source, &garbageCollector);

    if (fn == nullptr) {
        return InterpretResult::compileError;
    }

    stack.push_back(Value(fn));
    Closure* closure = garbageCollector.newClosure(fn);
    pop();
    push(Value(closure));
    call(closure, 0);

    InterpretResult result = run();

    garbageCollector.freeObjects(); // 
    return result;
}