#include <iostream>
#include "object.h"

Obj::Obj(ObjType type, Obj* next) : type(type), next(next) {}

ObjString::ObjString(const std::string& string, Obj* next) : string(string), obj(Obj(OBJ_STRING, next)) {}

ObjFunction::ObjFunction(Obj* next) : obj(Obj(OBJ_FUNCTION, next)) {}

bool Value::isString() {
    return type == VAL_OBJ && as.obj->type == OBJ_STRING;
}

bool Value::isFunction() {
    return type == VAL_OBJ && as.obj->type == OBJ_FUNCTION;
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

ObjFunction* Value::getObjFunction() {
    return (ObjFunction*)as.obj;
}

void Value::print() {
    switch (type) {
    case VAL_BOOL: std::cout << (as.boolean ? "true" : "false"); break;
    case VAL_NIL: std::cout << "nil"; break;
    case VAL_NUMBER: std::cout << as.number; break;
    case VAL_OBJ:
        switch (as.obj->type) {
        case OBJ_STRING: std::cout << getString(); break;
        case OBJ_FUNCTION:
            std::cout << "<fn " << getObjFunction()->name << ">";
            break;
        }

    }

}