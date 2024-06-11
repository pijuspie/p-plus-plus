#ifndef memory_h
#define memory_h

#include "value.h"

void freeObjects(Object* objects);
void collectGarbage();

#endif