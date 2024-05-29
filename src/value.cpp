#include <iostream>
#include "value.h"

Obj::Obj(ObjType type, Obj* next) : type(type), next(next) {}

ObjString::ObjString(const std::string& string, Obj* next) : string(string), obj(Obj(OBJ_STRING, next)) {}

ObjFunction::ObjFunction(const std::string& name, Obj* next) : name(name), obj(Obj(OBJ_FUNCTION, next)) {}

Value::Value(bool boolean) : type(VAL_BOOL) {
    as.boolean = boolean;
}

Value::Value(double number) : type(VAL_NUMBER) {
    as.number = number;
}

Value::Value(Obj* obj) : type(VAL_OBJ) {
    as.obj = obj;
}

Value::Value() : type(VAL_NIL) {
    as.number = 0;
};

bool Value::isBool() {
    return type == VAL_BOOL;
}

bool Value::isNumber() {
    return type == VAL_NUMBER;
}

bool Value::isObj() {
    return type == VAL_OBJ;
}

bool Value::isString() {
    return type == VAL_OBJ && as.obj->type == OBJ_STRING;
}

bool Value::isNil() {
    return type == VAL_NIL;
}

bool Value::getBool() {
    return as.boolean;
}

double Value::getNumber() {
    return as.number;
}

Obj* Value::getObj() {
    return as.obj;
}

ObjType Value::getObjType() {
    return as.obj->type;
}

ObjString* Value::getObjString() {
    return (ObjString*)as.obj;
}

std::string Value::getString() {
    return getObjString()->string;
}

void Value::print() {
    switch (type) {
    case VAL_BOOL: std::cout << (as.boolean ? "true" : "false"); break;
    case VAL_NIL: std::cout << "nil"; break;
    case VAL_NUMBER: std::cout << as.number; break;
    case VAL_OBJ:
        switch (as.obj->type) {
        case OBJ_STRING: std::cout << getString(); break;
        }
    }
}