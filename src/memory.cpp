#include "memory.h"
#include <iostream>

const bool debugAllocation = false;
const bool debugGC = false;

void GC::markObject(Object* object) {
    if (object == nullptr) return;
    if (object->isMarked) return;
    if (debugGC) {
        if (object->type == ObjectType::String) {
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
        if (object->type == ObjectType::String) {
            std::cout << object << " blacken: `" << Value(object).stringify() << "`" << std::endl;
        } else {
            std::cout << object << " blacken: " << Value(object).stringify() << std::endl;
        }
    }

    switch (object->type) {
    case ObjectType::Closure: {
        Closure* closure = (Closure*)object;
        markObject((Object*)closure->function);
        for (Upvalue* upvalue : closure->upvalues) {
            markObject((Object*)upvalue);
        }
        break;
    }
    case ObjectType::Function: {
        Function* function = (Function*)object;
        for (Value constant : function->chunk.constants) {
            markValue(constant);
        }
        break;
    }
    case ObjectType::Upvalue:
        markValue(((Upvalue*)object)->closed);
        break;
    case ObjectType::Class:
        break;
    case ObjectType::Instance: {
        Instance* instance = (Instance*)object;
        markObject((Object*)instance->klass);
        for (auto field : instance->fields) {
            markValue(field.second);
        }
        break;
    }
    case ObjectType::Native:
    case ObjectType::String:
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
}

String* GC::newString(std::string& chars) {
    collectGarbage();
    bytesAllocated += sizeof(String);
    String* string = new String;
    string->object.type = ObjectType::String;
    string->object.next = objects;
    objects = &string->object;
    string->chars = chars;
    if (debugAllocation) {
        std::cout << string << " allocate for: `" << Value(string).stringify() << "`" << std::endl;
    }
    return string;
}

Function* GC::newFunction(std::string& name) {
    collectGarbage();
    bytesAllocated += sizeof(Function);
    Function* function = new Function;
    function->object.type = ObjectType::Function;
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
    collectGarbage();
    bytesAllocated += sizeof(Native);
    Native* native = new Native;
    native->object.type = ObjectType::Native;
    native->object.next = objects;
    objects = &native->object;
    native->function = function;
    if (debugAllocation) {
        std::cout << native << " allocate for: `" << Value(native).stringify() << "`" << std::endl;
    }
    return native;
}

Closure* GC::newClosure(Function* function) {
    collectGarbage();
    bytesAllocated += sizeof(Closure);
    Closure* closure = new Closure;
    closure->object.type = ObjectType::Closure;
    closure->object.next = objects;
    objects = &closure->object;
    closure->function = function;
    if (debugAllocation) {
        std::cout << closure << " allocate for: `" << Value(closure).stringify() << "`" << std::endl;
    }
    return closure;
}

Upvalue* GC::newUpvalue(Value* location, Upvalue* next) {
    collectGarbage();
    bytesAllocated += sizeof(Upvalue);
    Upvalue* upvalue = new Upvalue;
    upvalue->object.type = ObjectType::Upvalue;
    upvalue->object.next = objects;
    objects = &upvalue->object;
    upvalue->location = location;
    upvalue->next = next;
    if (debugAllocation) {
        std::cout << upvalue << " allocate for: `" << Value(upvalue).stringify() << "`" << std::endl;
    }
    return upvalue;
}

Class* GC::newClass(std::string& name) {
    collectGarbage();
    bytesAllocated += sizeof(Class);
    Class* klass = new Class;
    klass->object.type = ObjectType::Class;
    klass->object.next = objects;
    objects = &klass->object;
    klass->name = name;
    if (debugAllocation) {
        std::cout << klass << " allocate for: `" << Value(klass).stringify() << "`" << std::endl;
    }
    return klass;
}

Instance* GC::newInstance(Class* klass) {
    collectGarbage();
    bytesAllocated += sizeof(Instance);
    Instance* instance = new Instance;
    instance->object.type = ObjectType::Instance;
    instance->object.next = objects;
    objects = &instance->object;
    instance->klass = klass;
    if (debugAllocation) {
        std::cout << instance << " allocate for: `" << Value(instance).stringify() << "`" << std::endl;
    }
    return instance;
}

void GC::freeObject(Object* object) {
    switch (object->type) {
    case ObjectType::String: {
        bytesAllocated -= sizeof(String);
        if (debugAllocation) std::cout << object << " free for: " << Value((String*)object).stringify() << std::endl;
        delete (String*)object; break;
    }
    case ObjectType::Function: {
        bytesAllocated -= sizeof(Function);
        if (debugAllocation) std::cout << object << " free for: " << Value((Function*)object).stringify() << std::endl;
        delete (Function*)object; break;
    }
    case ObjectType::Native: {
        bytesAllocated -= sizeof(Native);
        if (debugAllocation) std::cout << object << " free for: " << Value((Native*)object).stringify() << std::endl;
        delete (Native*)object; break;
    }
    case ObjectType::Closure: {
        bytesAllocated -= sizeof(Closure);
        if (debugAllocation) std::cout << object << " free for: " << Value((Closure*)object).stringify() << std::endl;
        delete (Closure*)object; break;
    }
    case ObjectType::Upvalue: {
        bytesAllocated -= sizeof(Upvalue);
        if (debugAllocation) std::cout << object << " free for: " << Value((Upvalue*)object).stringify() << std::endl;
        delete (Upvalue*)object; break;
    }
    case ObjectType::Class: {
        bytesAllocated -= sizeof(Class);
        if (debugAllocation) std::cout << object << " free for: " << Value((Class*)object).stringify() << std::endl;
        delete (Class*)object; break;
    }
    case ObjectType::Instance: {
        bytesAllocated -= sizeof(Instance);
        if (debugAllocation) std::cout << object << " free for: " << Value((Instance*)object).stringify() << std::endl;
        delete (Instance*)object; break;
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
