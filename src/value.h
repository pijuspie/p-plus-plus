#ifndef value_h
#define value_h

typedef struct Obj Obj;
typedef struct ObjString ObjString;

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