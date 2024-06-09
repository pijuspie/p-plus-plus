#ifndef value_h
#define value_h

#include <string>
#include <vector>

typedef struct Value Value;
typedef struct VM VM;

enum ValueType {
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING,
    VAL_FUNCTION,
    VAL_NATIVE,
    VAL_CLOSURE,
    VAL_UPVALUE,
};

struct Obj {
    ValueType type;
    Obj* next;
    Obj(ValueType type, Obj* next);
};

typedef bool (VM::* NativeFn)(int argCount, Value* args);

struct Native {
    Obj obj;
    NativeFn function;
    Native(NativeFn function, Obj* next);
};

Native& getNative(Value value);

struct ObjString {
    Obj obj;
    std::string string;
    ObjString(const std::string& string, Obj* next);
};

ObjString& getObjString(Value value);
std::string& getString(Value value);

struct ObjUpvalue {
    Obj obj;
    Value* location;
    ObjUpvalue(Value* location, Obj* obj);
};

enum OpCode {
    OP_CALL,
    OP_CLOSURE,
    OP_RETURN,
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
};

std::string getOpCode(OpCode opCode);

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;
};

struct Function {
    Obj obj;
    int arity = 0;
    int upvalueCount = 0;
    Chunk chunk;
    std::string name;
    Function(Obj* next);
};

Function& getFunction(Value value);

struct Closure {
    Obj obj;
    Function* function;
    std::vector<ObjUpvalue*> upvalues;
    Closure(Function* function, Obj* obj);
};

Closure& getClosure(Value value);

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* object;
    } as;

    Value();
    Value(bool boolean);
    Value(double number);
    Value(ObjString& string);
    Value(Function& function);
    Value(Native& native);
    Value(Closure& closure);
};

std::string stringify(Value value);

#endif