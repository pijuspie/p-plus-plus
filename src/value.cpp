#include <iostream>
#include "value.h"

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

bool Value::isNil() {
    return type == VAL_NIL;
}

bool Value::getBool() {
    return as.boolean;
}

double Value::getNumber() {
    return as.number;
}