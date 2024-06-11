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