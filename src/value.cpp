#include "value.h"
#include <iostream>

Obj::Obj(ValueType type1, Obj* next1) {
    type = type1;
    next = next1;
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

std::string stringify(Value value) {
    switch (value.type) {
    case VAL_NIL: return "nil";
    case VAL_BOOL: return (value.as.boolean ? "true" : "false");
    case VAL_NUMBER: return std::to_string(value.as.number);
    case VAL_STRING: return getString(value);
    case VAL_FUNCTION: return "<fn " + getFunction(value).name + ">";
    }

    return "unexpected type";
}