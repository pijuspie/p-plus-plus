#include <iostream>
#include <time.h>
#include <cmath>
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
    garbageCollector.initString = &initString;

    std::string init = "init";
    initString = garbageCollector.newString(init);

    stack.reserve(256);
    defineNative("clock", clockNative);
    defineNative("readNumber", readNumberNative);
    defineNative("stringify", stringifyNative);
    defineNative("round", roundNative);
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

bool VM::roundNative(int argCount, Value* args) {
    if (argCount != 2) {
        runtimeError("Expected 2 arguments but got " + std::to_string(argCount) + ".");
        return false;
    }

    Value number = args[0];
    Value precision = args[1];

    if (number.type != ValueType::number || precision.type != ValueType::number) {
        runtimeError("Arguments should be numbers.");
        return false;
    }

    double result = std::round(number.as.number / precision.as.number) * precision.as.number;
    push(Value(result));
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
    case ObjectType::Native: {
        NativeFn native = callee.getNative()->function;
        if (!(this->*native)(argCount, &stack[stack.size() - argCount])) {
            return false;
        }
        Value result = pop();
        stack.resize(stack.size() - argCount - 1);
        push(result);
        return true;
    }
    case ObjectType::Closure:
        return call(callee.getClosure(), argCount);
    case ObjectType::Class: {
        Class* klass = callee.getClass();
        stack[stack.size() - argCount - 1] = Value(garbageCollector.newInstance(klass));
        auto x = initString == nullptr ? klass->methods.end() : klass->methods.find(initString->chars);
        if (x != klass->methods.end()) {
            return call(x->second.getClosure(), argCount);
        } else if (argCount != 0) {
            runtimeError("Expected 0 arguments but got " + std::to_string(argCount) + ".");
            return false;
        }
        return true;
    }
    case ObjectType::BoundMethod: {
        BoundMethod* bound = callee.getBoundMethod();
        stack[stack.size() - argCount - 1] = bound->receiver;
        return call(bound->method, argCount);
    }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

bool VM::invokeFromClass(Class* klass, std::string name, int argCount) {
    Value method;
    if (klass == nullptr) {
        runtimeError("Undefined property '" + name + "'.");
        return false;
    }

    auto x = klass->methods.find(name);
    if (x == klass->methods.end()) {
        runtimeError("Undefined property '" + name + "'.");
        return false;
    }
    return call(x->second.getClosure(), argCount);
}

bool VM::invoke(Value receiver, std::string name, int argCount) {
    if (receiver.type != ValueType::object || receiver.as.object->type != ObjectType::Instance) {
        runtimeError("Only instances have methods.");
        return false;
    }

    Instance* instance = receiver.getInstance();

    auto x = instance->fields.find(name);
    if (x != instance->fields.end()) {
        stack[stack.size() - argCount - 1] = x->second;
        return callValue(x->second, argCount);
    }

    return invokeFromClass(instance->klass, name, argCount);
}

bool VM::bindMethod(Class* klass, std::string name) {
    if (klass == nullptr) {
        runtimeError("Undefined property '" + name + "'.");
        return false;
    }

    auto x = klass->methods.find(name);
    if (x == klass->methods.end()) {
        runtimeError("Undefined property '" + name + "'.");
        return false;
    }

    BoundMethod* bound = garbageCollector.newBoundMethod(peek(0), x->second.getClosure());
    pop();
    push(Value(bound));
    return true;
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

void VM::defineMethod(String* name) {
    Value method = peek(0);
    Class* klass = peek(1).getClass();
    Table::iterator value = klass->methods.find(name->chars);

    if (value != klass->methods.end()) {
        value->second = method;
    } else {
        klass->methods.insert({ name->chars, method });
    }

    pop();
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
        case ObjectType::String: return a.getString()->chars == b.getString()->chars;
        case ObjectType::Function: return a.getFunction()->name == b.getFunction()->name;
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
        case OP_INVOKE: {
            String* method = readConstant().getString();
            int argCount = readByte();
            Value receiver = peek(argCount);
            if (!invoke(receiver, method->chars, argCount)) {
                return InterpretResult::runtimeError;
            }
            frame = &frames[frames.size() - 1];
            break;
        }
        case OP_INVOKE_BY_KEY: {
            int argCount = readByte();
            Value method = peek(argCount);
            std::string name;

            if (method.type == ValueType::number) {
                name = method.stringify();
            } else if (method.type == ValueType::object && method.as.object->type == ObjectType::String) {
                name = method.stringify();
            } else {
                runtimeError("A key must be a number or a string.");
                return InterpretResult::runtimeError;
            }

            Value receiver = peek(argCount + 1);
            if (!invoke(receiver, name, argCount)) {
                return InterpretResult::runtimeError;
            }

            Value value = pop();
            pop();
            push(value);

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
        case OP_GET_PROPERTY: {
            if (peek(0).type != ValueType::object || peek(0).as.object->type != ObjectType::Instance) {
                runtimeError("Only instances have properties.");
                return InterpretResult::runtimeError;
            }

            Instance* instance = peek(0).getInstance();
            String* name = readConstant().getString();

            auto x = instance->fields.find(name->chars);
            if (x != instance->fields.end()) {
                pop();
                push(x->second);
                break;
            }

            if (!bindMethod(instance->klass, name->chars)) {
                return InterpretResult::runtimeError;
            }
            break;
        }
        case OP_SET_PROPERTY: {
            if (peek(1).type != ValueType::object || peek(1).as.object->type != ObjectType::Instance) {
                runtimeError("Only instances have fields.");
                return InterpretResult::runtimeError;
            }

            Instance* instance = peek(1).getInstance();

            std::string name = readConstant().getString()->chars;
            std::unordered_map<std::string, Value>::iterator x = instance->fields.find(name);

            if (x != globals.end()) {
                x->second = peek(0);
            } else {
                instance->fields.insert({ name, peek(0) });
            }

            Value value = pop();
            pop();
            push(value);
            break;
        }
        case OP_GET_PROPERTY_BY_KEY: {
            if (peek(1).type != ValueType::object || peek(1).as.object->type != ObjectType::Instance) {
                runtimeError("Only instances have properties.");
                return InterpretResult::runtimeError;
            }

            Instance* instance = peek(1).getInstance();
            std::string name;

            if (peek(0).type == ValueType::number) {
                name = peek(0).stringify();
            } else if (peek(0).type == ValueType::object && peek(0).as.object->type == ObjectType::String) {
                name = peek(0).stringify();
            } else {
                runtimeError("A key must be a number or a string.");
                return InterpretResult::runtimeError;
            }
            pop();

            auto x = instance->fields.find(name);
            if (x != instance->fields.end()) {
                pop();
                push(x->second);
                break;
            }

            if (!bindMethod(instance->klass, name)) {
                return InterpretResult::runtimeError;
            }
            break;
        }
        case OP_SET_PROPERTY_BY_KEY: {
            if (peek(2).type != ValueType::object || peek(2).as.object->type != ObjectType::Instance) {
                runtimeError("Only instances have fields.");
                return InterpretResult::runtimeError;
            }

            Instance* instance = peek(2).getInstance();
            std::string name;

            if (peek(1).type == ValueType::number) {
                name = peek(1).stringify();
            } else if (peek(1).type == ValueType::object && peek(1).as.object->type == ObjectType::String) {
                name = peek(1).stringify();
            } else {
                runtimeError("A key must be a number or a string.");
                return InterpretResult::runtimeError;
            }

            std::unordered_map<std::string, Value>::iterator x = instance->fields.find(name);

            if (x != globals.end()) {
                x->second = peek(0);
            } else {
                instance->fields.insert({ name, peek(0) });
            }

            Value value = pop();
            pop();
            pop();
            push(value);
            break;
        }
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
            if (peek(0).type == ValueType::object && peek(0).as.object->type == ObjectType::String &&
                peek(1).type == ValueType::object && peek(1).as.object->type == ObjectType::String) {
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
        case OP_REMAIN: {
            if (peek(0).type != ValueType::number || peek(1).type != ValueType::number) {
                runtimeError("Operands must be numbers.");
                return InterpretResult::runtimeError;
            }

            Value b = pop();
            Value a = pop();
            double result = std::fmod(a.as.number, b.as.number);

            push(Value(result));
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
        case OP_PRINTL: {
            std::cout << pop().stringify() << std::endl;
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
        case OP_CLASS:
            push(Value(garbageCollector.newClass(readConstant().getString()->chars)));
            break;
        case OP_METHOD:
            defineMethod(readConstant().getString());
            break;
        case OP_ARRAY: {
            int itemCount = readByte();
            Instance* instance = garbageCollector.newInstance(nullptr);
            for (int i = itemCount - 1; i >= 0; i--) {
                instance->fields.insert({ std::to_string(i), pop() });
            }
            push(Value(instance));
            break;
        }
        case OP_MAP: {
            Instance* instance = garbageCollector.newInstance(nullptr);
            push(Value(instance));
            break;
        }
        case OP_KEY: {
            Value value = pop();
            Value instance = pop();
            std::string key = readConstant().getString()->chars;
            instance.getInstance()->fields.insert({ key, value });
            push(instance);
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

    return run();
}

VM::~VM() {
    initString = nullptr;
    garbageCollector.freeObjects();
}