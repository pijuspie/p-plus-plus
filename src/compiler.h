#ifndef compiler_h
#define compiler_h

#include "scanner.h"
#include "value.h"
#include "memory.h"
#include <string>


Function* compile(const std::string& source, GC* garbageCollector);

#endif 