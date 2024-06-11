#ifndef value2_h
#define value2_h

#include <string>
#include <vector>

typedef struct VM VM;
typedef struct String String;
typedef struct Function Function;
typedef struct Native Native;
typedef struct Closure Closure;

enum class ObjectType {
    string,
    function,
    native,
    upvalue,
    closure,
};

struct Object {
    ObjectType type;
    Object* next;
};

enum class ValueType {
    nil,
    boolean,
    number,
    object,
};

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Object* object;
    } as;

    Value();
    Value(bool boolean);
    Value(double number);
    Value(String* string);
    Value(Function* function);
    Value(Native* native);
    Value(Closure* closure);

    String* getString();
    Function* getFunction();
    Native* getNative();
    Closure* getClosure();

    std::string stringify();
};

struct String {
    Object object;
    std::string chars;
};

enum OpCode {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
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
    OP_CALL,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
};

std::string stringifyOpCode(OpCode opCode);

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;
};

struct Function {
    Object object;
    std::string name;
    int arity;
    int upvalueCount;
    Chunk chunk;
};

typedef bool (VM::* NativeFn)(int argCount, Value* args);

struct Native {
    Object object;
    NativeFn function;
};

struct Upvalue {
    Object object;
    Value* location;
    Value closed;
    Upvalue* next;
};

struct Closure {
    Object object;
    Function* function;
    std::vector<Upvalue*> upvalues;
};

#endif