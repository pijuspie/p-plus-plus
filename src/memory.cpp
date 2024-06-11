#include "memory.h"
#include <iostream>

const bool debugAllocation = false;
const bool debugGC = false;

void GC::collectGarbage() {
    if (bytesAllocated < nextGC && !debugGC) {
        return;
    }

    if (debugGC) std::cout << "collecting garbage" << std::endl;


}

String* GC::newString(std::string& chars) {
    bytesAllocated += sizeof(String);
    collectGarbage();
    String* string = new String;
    string->object.type = ObjectType::string;
    string->object.next = objects;
    objects = &string->object;
    string->chars = chars;
    if (debugAllocation) {
        std::cout << "allocated string: `" << Value(string).stringify() << "`" << std::endl;
    }
    return string;
}

Function* GC::newFunction(std::string& name) {
    bytesAllocated += sizeof(Function);
    collectGarbage();
    Function* function = new Function;
    function->object.type = ObjectType::function;
    function->object.next = objects;
    objects = &function->object;
    function->name = name;
    function->arity = 0;
    function->upvalueCount = 0;
    if (debugAllocation) {
        std::cout << "allocated function: " << Value(function).stringify() << std::endl;
    }
    return function;
}

Native* GC::newNative(NativeFn function) {
    bytesAllocated += sizeof(Native);
    collectGarbage();
    Native* native = new Native;
    native->object.type = ObjectType::native;
    native->object.next = objects;
    objects = &native->object;
    native->function = function;
    if (debugAllocation) {
        std::cout << "allocated native: " << Value(native).stringify() << std::endl;
    }
    return native;
}

Closure* GC::newClosure(Function* function) {
    bytesAllocated += sizeof(Closure);
    collectGarbage();
    Closure* closure = new Closure;
    closure->object.type = ObjectType::closure;
    closure->object.next = objects;
    objects = &closure->object;
    closure->function = function;
    if (debugAllocation) {
        std::cout << "allocated closure: " << Value(closure).stringify() << std::endl;
    }
    return closure;
}

Upvalue* GC::newUpvalue(Value* location, Upvalue* next) {
    bytesAllocated += sizeof(Upvalue);
    collectGarbage();
    Upvalue* upvalue = new Upvalue;
    upvalue->object.type = ObjectType::upvalue;
    upvalue->object.next = objects;
    objects = &upvalue->object;
    upvalue->location = location;
    upvalue->next = next;
    if (debugAllocation) {
        std::cout << "allocated upvalue: " << Value(upvalue).stringify() << std::endl;
    }
    return upvalue;
}

void GC::freeObjects() {
    if (debugAllocation) {
        std::cout << std::endl << "Freeing objects:" << std::endl;
    }

    while (objects != nullptr) {
        Object* next = objects->next;

        switch (objects->type) {
        case ObjectType::string: {
            if (debugAllocation) std::cout << "string: `" << Value((String*)objects).stringify() << "`" << std::endl;
            delete (String*)objects; break;
        }
        case ObjectType::function: {
            if (debugAllocation) std::cout << "function: " << Value((Function*)objects).stringify() << std::endl;
            delete (Function*)objects; break;
        }
        case ObjectType::native: {
            if (debugAllocation) std::cout << "native: " << Value((Native*)objects).stringify() << std::endl;
            delete (Native*)objects; break;
        }
        case ObjectType::closure: {
            if (debugAllocation) std::cout << "closure: " << Value((Closure*)objects).stringify() << std::endl;
            delete (Closure*)objects; break;
        }
        case ObjectType::upvalue: {
            if (debugAllocation) std::cout << "upvalue: " << Value((Upvalue*)objects).stringify() << std::endl;
            delete (Upvalue*)objects; break;
        }
        }

        objects = next;
    }
}
