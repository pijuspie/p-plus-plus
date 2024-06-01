#ifndef value_h
#define value_h

#include <string>

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjFunction ObjFunction;

enum ObjType {
    OBJ_STRING,
    OBJ_FUNCTION,
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
    bool isFunction();
    bool isNil();

    bool getBool();
    double getNumber();
    Obj* getObj();
    ObjType getObjType();
    ObjString* getObjString();
    std::string getString();
    ObjFunction* getObjFunction();

    void print();
};

#endif