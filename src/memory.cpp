#include "memory.h"
#include <iostream>

const bool debugAllocation = false;
const bool debugGC = false;

void GC::markObject(Object* object) {
    if (object == nullptr) return;
    if (object->isMarked) return;
    if (debugGC) {
        if (object->type == ObjectType::string) {
            std::cout << object << " mark: `" << Value(object).stringify() << "`" << std::endl;
        } else {
            std::cout << object << " mark: " << Value(object).stringify() << std::endl;
        }
    }
    object->isMarked = true;
    grayObjects.push_back(object);
}

void GC::markValue(Value value) {
    if (value.type == ValueType::object) markObject(value.as.object);
}

void GC::markRoots() {
    for (Value slot : *stack) {
        markValue(slot);
    }

    for (Upvalue* upvalue = *openUpvalues; upvalue != nullptr; upvalue = upvalue->next) {
        markObject((Object*)upvalue);
    }

    for (auto global : *globals) {
        markValue(global.second);
    }

    for (CallFrame frame : *frames) {
        markObject((Object*)frame.closure);
    }

    Compiler* current = compiler;
    while (current != nullptr) {
        markObject((Object*)current->function);
        current = current->enclosing;
    }
}

void GC::blackenObject(Object* object) {
    if (debugGC) {
        if (object->type == ObjectType::string) {
            std::cout << object << " blacken: `" << Value(object).stringify() << "`" << std::endl;
        } else {
            std::cout << object << " blacken: " << Value(object).stringify() << std::endl;
        }
    }

    switch (object->type) {
    case ObjectType::closure: {
        Closure* closure = (Closure*)object;
        markObject((Object*)closure->function);
        for (Upvalue* upvalue : closure->upvalues) {
            markObject((Object*)upvalue);
        }
        break;
    }
    case ObjectType::function: {
        Function* function = (Function*)object;
        for (Value constant : function->chunk.constants) {
            markValue(constant);
        }
        break;
    }
    case ObjectType::upvalue:
        markValue(((Upvalue*)object)->closed);
        break;
    case ObjectType::native:
    case ObjectType::string:
        break;

    }
}

void GC::traceReferences() {
    while (grayObjects.size() > 0) {
        Object* object = grayObjects[grayObjects.size() - 1];
        grayObjects.pop_back();
        blackenObject(object);
    }
}

void GC::sweep() {
    Object* previous = nullptr;
    Object* object = objects;
    while (object != nullptr) {
        if (object->isMarked) {
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Object* unreached = object;
            object = object->next;
            if (previous != nullptr) {
                previous->next = object;
            } else {
                objects = object;
            }

            freeObject(unreached);
        }
    }
}

void GC::collectGarbage() {
    if (bytesAllocated < nextGC && !debugGC) {
        return;
    }

    if (debugGC) {
        std::cout << std::endl << "-- gc begin" << std::endl;
    }

    size_t before = bytesAllocated;

    markRoots();
    traceReferences();
    sweep();

    nextGC = bytesAllocated * 2;

    if (debugGC) {
        std::cout << "-- gc end" << std::endl;
        std::cout << "   collected " << before - bytesAllocated << " bytes (from " << before << " to " << bytesAllocated << ") next at " << nextGC << std::endl;
    }

    // if (before - bytesAllocated > 0) {
    //     std::cout << "Collected " << before - bytesAllocated << " bytes" << std::endl;
    // }
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
        std::cout << string << " allocate for: `" << Value(string).stringify() << "`" << std::endl;
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
        std::cout << function << " allocate for: `" << Value(function).stringify() << "`" << std::endl;
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
        std::cout << native << " allocate for: `" << Value(native).stringify() << "`" << std::endl;
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
        std::cout << closure << " allocate for: `" << Value(closure).stringify() << "`" << std::endl;
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
        std::cout << upvalue << " allocate for: `" << Value(upvalue).stringify() << "`" << std::endl;
    }
    return upvalue;
}

void GC::freeObject(Object* object) {
    switch (object->type) {
    case ObjectType::string: {
        bytesAllocated -= sizeof(String);
        if (debugAllocation) std::cout << object << " free for: " << Value((String*)object).stringify() << std::endl;
        delete (String*)object; break;
    }
    case ObjectType::function: {
        bytesAllocated -= sizeof(Function);
        if (debugAllocation) std::cout << object << " free for: " << Value((Function*)object).stringify() << std::endl;
        delete (Function*)object; break;
    }
    case ObjectType::native: {
        bytesAllocated -= sizeof(Native);
        if (debugAllocation) std::cout << object << " free for: " << Value((Native*)object).stringify() << std::endl;
        delete (Native*)object; break;
    }
    case ObjectType::closure: {
        bytesAllocated -= sizeof(Closure);
        if (debugAllocation) std::cout << object << " free for: " << Value((Closure*)object).stringify() << std::endl;
        delete (Closure*)object; break;
    }
    case ObjectType::upvalue: {
        bytesAllocated -= sizeof(Upvalue);
        if (debugAllocation) std::cout << object << " free for: " << Value((Upvalue*)object).stringify() << std::endl;
        delete (Upvalue*)object; break;
    }
    }
}

void GC::freeObjects() {
    if (debugAllocation) {
        std::cout << std::endl << "Freeing objects:" << std::endl;
    }

    while (objects != nullptr) {
        Object* next = objects->next;
        freeObject(objects);
        objects = next;
    }
}
