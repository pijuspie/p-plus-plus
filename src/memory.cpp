#include "memory.h"

void freeObjects(Object* objects) {
    while (objects != nullptr) {
        Object* next = objects->next;

        switch (objects->type) {
        case ObjectType::string: delete (String*)objects; break;
        case ObjectType::function: delete (Function*)objects; break;
        case ObjectType::native: delete (Native*)objects; break;
        case ObjectType::closure: delete (Closure*)objects; break;
        case ObjectType::upvalue: delete (Upvalue*)objects; break;
        }

        objects = next;
    }
}

void collectGarbage() {

}
