#include <iostream>
#include <fstream>
#include <string>
#include "vm.h"

void repl() {
    std::string line;
    for (;;) {
        std::cout << "> ";

        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;
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

    std::string source;
    file.seekg(0, std::ios::end);
    source.resize(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(&source[0], source.size());
    file.close();

    switch (interpret(source)) {
    case InterpretResult::compileError:
        std::cout << "compile" << std::endl;
        return 65;
    case InterpretResult::runtimeError:
        std::cout << "runtime" << std::endl;
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