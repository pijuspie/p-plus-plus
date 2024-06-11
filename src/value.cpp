#include "value.h"

Value::Value() {
    type = ValueType::nil;
}

Value::Value(bool boolean) {
    type = ValueType::boolean;
    as.boolean = boolean;
}

Value::Value(double number) {
    type = ValueType::number;
    as.number = number;
}

Value::Value(String* string) {
    type = ValueType::object;
    as.object = (Object*)string;
}

Value::Value(Function* function) {
    type = ValueType::object;
    as.object = (Object*)function;
}

Value::Value(Native* native) {
    type = ValueType::object;
    as.object = (Object*)native;
}

Value::Value(Closure* closure) {
    type = ValueType::object;
    as.object = (Object*)closure;
}

String* Value::getString() { return (String*)as.object; }
Function* Value::getFunction() { return (Function*)as.object; }
Native* Value::getNative() { return (Native*)as.object; }
Closure* Value::getClosure() { return (Closure*)as.object; }

std::string Value::stringify() {
    switch (type) {
    case ValueType::nil: return "nil";
    case ValueType::boolean: return (as.boolean ? "true" : "false");
    case ValueType::number: return std::to_string(as.number);
    case ValueType::object: {
        switch (as.object->type) {
        case ObjectType::string: return this->getString()->chars;
        case ObjectType::native: return "<native fn>";
        case ObjectType::closure: {
            Function* fn = this->getClosure()->function;
            if (fn->name == "") return "<script>";
            else return "<fn " + fn->name + ">";
        }
        case ObjectType::function: {
            Function* fn = this->getFunction();
            if (fn->name == "") return "<script>";
            else return "<fn " + fn->name + ">";
        }
        case ObjectType::upvalue: return "upvalue";
        }
    }
    }

    return "unexpected type";
}

String* newString(std::string& chars, Object*& next) {
    String* string = new String;
    string->object.type = ObjectType::string;
    string->object.next = next;
    next = &string->object;
    string->chars = chars;
    return string;
}

std::string stringifyOpCode(OpCode opCode) {
    switch (opCode) {
    case OP_CONSTANT: return "CONSTANT";
    case OP_NIL: return "NIL";
    case OP_TRUE: return "TRUE";
    case OP_FALSE: return "FALSE";
    case OP_POP: return "POP";
    case OP_GET_LOCAL: return "GET_LOCAL";
    case OP_SET_LOCAL: return "SET_LOCAL";
    case OP_GET_GLOBAL: return "GET_GLOBAL";
    case OP_DEFINE_GLOBAL: return "DEFINE_GLOBAL";
    case OP_SET_GLOBAL: return "SET_GLOBAL";
    case OP_GET_UPVALUE: return "GET_UPVALUE";
    case OP_SET_UPVALUE: return "SET_UPVALUE";
    case OP_EQUAL: return "EQUAL";
    case OP_GREATER: return "GREATER";
    case OP_LESS: return "LESS";
    case OP_ADD: return "ADD";
    case OP_SUBTRACT: return "SUBTRACT";
    case OP_MULTIPLY: return "MULTIPLY";
    case OP_DIVIDE: return "DIVIDE";
    case OP_NOT: return "NOT";
    case OP_NEGATE: return "NEGATE";
    case OP_PRINT: return "PRINT";
    case OP_JUMP: return "JUMP";
    case OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
    case OP_LOOP: return "LOOP";
    case OP_CALL: return "CALL";
    case OP_CLOSURE: return "CLOSURE";
    case OP_CLOSE_UPVALUE: return "CLOSE_UPVALUE";
    case OP_RETURN: return "RETURN";
    default: return "Unexpected code: " + std::to_string(opCode);
    }
}

Function* newFunction(std::string& name, Object*& next) {
    Function* function = new Function;
    function->object.type = ObjectType::function;
    function->object.next = next;
    next = &function->object;
    function->name = name;
    function->arity = 0;
    function->upvalueCount = 0;
    return function;
}

Native* newNative(NativeFn function, Object*& next) {
    Native* native = new Native;
    native->object.type = ObjectType::native;
    native->object.next = next;
    next = &native->object;
    native->function = function;
    return native;
}

Closure* newClosure(Function* function, Object*& next) {
    Closure* closure = new Closure;
    closure->object.type = ObjectType::closure;
    closure->object.next = next;
    next = &closure->object;
    closure->function = function;
    return closure;
}

Upvalue* newUpvalue(Value* location, Upvalue* next, Object*& objects) {
    Upvalue* upvalue = new Upvalue;
    upvalue->object.type = ObjectType::upvalue;
    upvalue->object.next = objects;
    objects = &upvalue->object;
    upvalue->location = location;
    upvalue->next = next;
    return upvalue;
}