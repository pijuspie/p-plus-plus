#include "value.h"
#include <iostream>

Obj::Obj(ValueType type1, Obj* next1) {
    type = type1;
    next = next1;
}

Native::Native(NativeFn function1, Obj* next) : obj(Obj(VAL_NATIVE, next)) {
    function = function1;
}

Native& getNative(Value value) {
    return *(Native*)value.as.object;
}

ObjString::ObjString(const std::string& string1, Obj* next) : obj(Obj(VAL_STRING, next)) {
    string = string1;
}

ObjString& getObjString(Value value) {
    return *(ObjString*)value.as.object;
}

std::string& getString(Value value) {
    return getObjString(value).string;
}

std::string getOpCode(OpCode opCode) {
    switch (opCode) {
    case OP_CALL: return "OP_CALL";
    case OP_RETURN: return "OP_RETURN";
    case OP_CONSTANT: return "OP_CONSTANT";
    case OP_NIL: return "OP_NIL";
    case OP_TRUE: return "OP_TRUE";
    case OP_FALSE: return "OP_FALSE";
    case OP_EQUAL: return "OP_EQUAL";
    case OP_GREATER: return "OP_GREATER";
    case OP_LESS: return "OP_LESS";
    case OP_ADD: return "OP_ADD";
    case OP_SUBTRACT: return "OP_SUBTRACT";
    case OP_MULTIPLY: return "OP_MULTIPLY";
    case OP_DIVIDE: return "OP_DIVIDE";
    case OP_NOT: return "OP_NOT";
    case OP_NEGATE: return "OP_NEGATE";
    case OP_PRINT: return "OP_PRINT";
    case OP_JUMP: return "OP_JUMP";
    case OP_JUMP_IF_FALSE: return "OP_JUMP_IF_FALSE";
    case OP_LOOP: return "OP_LOOP";
    case OP_POP: return "OP_POP";
    case OP_DEFINE_GLOBAL: return "OP_DEFINE_GLOBAL";
    case OP_GET_LOCAL: return "OP_GET_LOCAL";
    case OP_SET_LOCAL: return "OP_SET_LOCAL";
    case OP_GET_GLOBAL: return "OP_GET_GLOBAL";
    case OP_SET_GLOBAL: return "OP_SET_GLOBAL";
    default:
        return "Unexpected code";
    }
}

Function::Function(Obj* next) : obj(Obj(VAL_FUNCTION, next)) {}

Function& getFunction(Value value) {
    return *(Function*)value.as.object;
}

Value::Value() : type(VAL_NIL) {
    as.number = 0;
};

Value::Value(bool boolean) {
    type = VAL_BOOL;
    as.boolean = boolean;
}

Value::Value(double number) {
    type = VAL_NUMBER;
    as.number = number;
}

Value::Value(ObjString& string) {
    type = VAL_STRING;
    as.object = (Obj*)&string;
}

Value::Value(Function& function) {
    type = VAL_FUNCTION;
    as.object = (Obj*)&function;
}

Value::Value(Native& native) {
    type = VAL_NATIVE;
    as.object = (Obj*)&native;
}

std::string stringify(Value value) {
    switch (value.type) {
    case VAL_NIL: return "nil";
    case VAL_BOOL: return (value.as.boolean ? "true" : "false");
    case VAL_NUMBER: return std::to_string(value.as.number);
    case VAL_STRING: return getString(value);
    case VAL_NATIVE: return "<native fn>";
    case VAL_FUNCTION: {
        Function& fn = getFunction(value);
        if (fn.name == "") {
            return "<script>";
        } else {
            return "<fn " + fn.name + ">";
        }
    }
    }

    return "unexpected type";
}