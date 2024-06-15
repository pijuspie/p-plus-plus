#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "vm.h"

void repl() {
    std::string line;
    for (;;) {
        std::cout << "> ";

        if (!std::getline(std::cin, line)) {
            break;
        }

        interpret(line);
    }
}

int runFile(const char* path) {
    std::ifstream file(path);

    if (!file) {
        std::cerr << "Could not open file \"" << path << "\"." << std::endl;
        return 74;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    std::string source = buffer.str();

    switch (interpret(source)) {
    case InterpretResult::compileError:
        return 65;
    case InterpretResult::runtimeError:
        return 70;
    }
    return 0;
}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        repl();
        return 0;
    }

    if (argc == 2) {
        return runFile(argv[1]);
    }

    std::cerr << "Usage: clox [path]" << std::endl;
    return 64;
}