#ifndef value_h
#define value_h

#include "chunk.h"

enum ObjType {
    OBJ_STRING,
    OBJ_FUNCTION,
};

struct Obj {
    ObjType type;
    Obj* next;
    Obj(ObjType type, Obj* next);
};

struct ObjString {
    Obj obj;
    std::string string;
    ObjString(const std::string& string, Obj* next);
};

struct ObjFunction {
    Obj obj;
    int arity = 0;
    Chunk chunk;
    std::string name;

    ObjFunction::ObjFunction(const std::string& name, Obj* next);
};

enum ValueType {
    VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_OBJ
};

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;

    Value(bool boolean);
    Value(double number);
    Value(Obj* obj);
    Value();

    bool isBool();
    bool isNumber();
    bool isObj();
    bool isString();
    bool isNil();

    bool getBool();
    double getNumber();
    Obj* getObj();
    ObjType getObjType();
    ObjString* getObjString();
    std::string getString();

    void print();
};

#endif