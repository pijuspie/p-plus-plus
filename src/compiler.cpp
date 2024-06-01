#include "compiler.h"
#include "value.h"
#include "object.h"
#include <iostream>

Local::Local(Token name, int depth) : name(name), depth(depth) {}

enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_TERM,
    PREC_FACTOR,
    PREC_UNARY,
    PREC_CALL,
    PREC_PRIMARY
};

class Parser;

typedef void (Parser::* ParseFn)(bool canAssign);

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

class Parser {
private:
    Obj* objects;
    Compiler& compiler;

    int line = 1;
    StringIterator current_char;
    StringIterator end_char;

    Token current;
    Token previous;
    bool hadError = false;
    bool panicMode = false;

    Chunk& getChunk() {
        return compiler.function->chunk;
    }

    void errorAtCurrent(const std::string& message) {
        errorAt(&current, message);
    }

    void error(const std::string& message) {
        errorAt(&previous, message);
    }

    void errorAt(Token* token, const std::string& message) {
        if (panicMode) return;
        panicMode = true;

        std::cerr << "[line " << token->line << "] Error";

        if (token->type == TOKEN_EOF) {
            std::cerr << " at end";
        } else if (token->type != TOKEN_ERROR) {
            std::cerr << " at '" << std::string(token->start, token->end) << "'";
        }

        std::cerr << ": " << message << std::endl;
        hadError = true;
    }

    void advance() {
        previous = current;

        for (;;) {
            current = scanToken(current_char, end_char, line);
            if (current.type != TOKEN_ERROR) break;
            errorAtCurrent(std::string(current.start, current.end));
        }
    }

    void consume(TokenType type, const std::string& message) {
        if (current.type == type) {
            advance();
            return;
        }

        errorAtCurrent(message);
    }

    bool check(TokenType type) {
        return current.type == type;
    }

    bool match(TokenType type) {
        if (!check(type)) return false;
        advance();
        return true;
    }

    void emitByte(uint8_t byte) {
        getChunk().code.push_back(byte);
        getChunk().lines.push_back(previous.line);
    }

    void emitBytes(uint8_t byte1, uint8_t byte2) {
        emitByte(byte1);
        emitByte(byte2);
    }

    void emitLoop(int loopStart) {
        emitByte(OP_LOOP);

        int offset = getChunk().code.size() - loopStart + 2;
        if (offset > 65535) error("Loop body too large.");

        emitByte((offset >> 8) & 0xff);
        emitByte(offset & 0xff);
    }

    int emitJump(uint8_t instruction) {
        emitByte(instruction);
        emitByte(0xff);
        emitByte(0xff);
        return getChunk().code.size() - 2;
    }

    void emitConstant(Value value) {
        emitBytes(OP_CONSTANT, makeConstant(value));
    }

    void patchJump(int offset) {
        int jump = getChunk().code.size() - offset - 2;

        if (jump > 65535) {
            error("Too much code to jump over.");
        }

        getChunk().code[offset] = (jump >> 8) & 0xff;
        getChunk().code[offset + 1] = jump & 0xff;
    }

    uint8_t makeConstant(Value value) {
        getChunk().constants.push_back(value);
        int constant = getChunk().constants.size() - 1;

        if (constant > 255) {
            error("Too many constants in one chunk.");
            return 0;
        }

        return (uint8_t)constant;
    }

    void declaration() {
        if (match(TOKEN_VAR)) {
            varDeclaration();
        } else {
            statement();
        }

        if (panicMode) {
            synchronize();
        }
    }

    void varDeclaration() {
        uint8_t global = parseVariable("Expect variable name.");

        if (match(TOKEN_EQUAL)) {
            expression();
        } else {
            emitByte(OP_NIL);
        }

        consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
        defineVariable(global);
    }

    void statement() {
        if (match(TOKEN_PRINT)) {
            printStatement();
        } else if (match(TOKEN_IF)) {
            ifStatement();
        } else if (match(TOKEN_WHILE)) {
            whileStatement();
        } else if (match(TOKEN_FOR)) {
            forStatement();
        } else if (match(TOKEN_LEFT_BRACE)) {
            beginScope();
            block();
            endScope();
        } else {
            expressionStatement();
        }
    }

    void expression() {
        parsePrecedence(PREC_ASSIGNMENT);
    }

    void beginScope() {
        compiler.scopeDepth++;
    }

    void endScope() {
        compiler.scopeDepth--;
        while (compiler.locals.size() > 0 && compiler.locals[compiler.locals.size() - 1].depth > compiler.scopeDepth) {
            compiler.locals.pop_back();
            emitByte(OP_POP);
        }
    }

    void block() {
        while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
            declaration();
        }

        consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    }

    void printStatement() {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after value.");
        emitByte(OP_PRINT);
    }

    void forStatement() {
        beginScope();
        consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

        if (match(TOKEN_VAR)) {
            varDeclaration();
        } else if (!match(TOKEN_SEMICOLON)) {
            expressionStatement();
        }

        consume(TOKEN_SEMICOLON, "Expect ';'.");

        int loopStart = getChunk().code.size();
        int exitJump = -1;
        if (!match(TOKEN_SEMICOLON)) {
            expression();
            consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

            exitJump = emitJump(OP_JUMP_IF_FALSE);
            emitByte(OP_POP);
        }

        if (!match(TOKEN_RIGHT_PAREN)) {
            int bodyJump = emitJump(OP_JUMP);
            int incrementStart = getChunk().code.size();
            expression();
            emitByte(OP_POP);
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

            emitLoop(loopStart);
            loopStart = incrementStart;
            patchJump(bodyJump);
        }

        statement();
        emitLoop(loopStart);

        if (exitJump != -1) {
            patchJump(exitJump);
            emitByte(OP_POP);
        }

        endScope();
    }

    void whileStatement() {
        int loopStart = getChunk().code.size();
        consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        statement();
        emitLoop(loopStart);

        patchJump(exitJump);
        emitByte(OP_POP);
    }

    void expressionStatement() {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
        emitByte(OP_POP);
    }

    void ifStatement() {
        consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int thenJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
        statement();

        int elseJump = emitJump(OP_JUMP);
        patchJump(thenJump);
        emitByte(OP_POP);

        if (match(TOKEN_ELSE)) {
            statement();
        }

        patchJump(elseJump);
    }

    void synchronize() {
        panicMode = false;

        while (current.type != TOKEN_EOF) {
            if (previous.type == TOKEN_SEMICOLON) return;
            switch (current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                return;
            }

            advance();
        }
    }

    void number(bool canAssign) {
        double value = std::stod(std::string(previous.start, previous.end));
        emitConstant(value);
    }

    void or_(bool canAssign) {
        int elseJump = emitJump(OP_JUMP_IF_FALSE);
        int endJump = emitJump(OP_JUMP);

        patchJump(elseJump);
        emitByte(OP_POP);

        parsePrecedence(PREC_OR);
        patchJump(endJump);
    }

    void string(bool canAssign) {
        ObjString* value = new ObjString(std::string(previous.start + 1, previous.end - 1), objects);
        objects = (Obj*)value;
        emitConstant((Obj*)value);
    }

    void variable(bool canAssign) {
        namedVariable(previous, canAssign);
    }

    void namedVariable(Token name, bool canAssign) {
        uint8_t getOp, setOp;
        int arg = resolveLocal(&name);

        if (arg != -1) {
            getOp = OP_GET_LOCAL;
            setOp = OP_SET_LOCAL;
        } else {
            arg = identifierConstant(&name);
            getOp = OP_GET_GLOBAL;
            setOp = OP_SET_GLOBAL;
        }

        if (canAssign && match(TOKEN_EQUAL)) {
            expression();
            emitBytes(setOp, (uint8_t)arg);
        } else {
            emitBytes(getOp, (uint8_t)arg);
        }
    }

    void unary(bool canAssign) {
        TokenType operatorType = previous.type;

        parsePrecedence(PREC_UNARY);

        switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;
        }
    }

    void binary(bool canAssign) {
        TokenType operatorType = previous.type;
        ParseRule* rule = getRule(operatorType);
        parsePrecedence((Precedence)(rule->precedence + 1));

        switch (operatorType) {
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return;
        }
    }

    void literal(bool canAssign) {
        switch (previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default: return;
        }
    }

    void grouping(bool canAssign) {
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    }

    ParseRule rules[40] = {
        [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
        [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS] = {unary, binary, PREC_TERM},
        [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG] = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
        [TOKEN_STRING] = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
        [TOKEN_AND] = {NULL, and_, PREC_AND},
        [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
        [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
        [TOKEN_IF] = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL] = {literal, NULL, PREC_NONE},
        [TOKEN_OR] = {NULL, or_, PREC_OR},
        [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
        [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    };

    ParseRule* getRule(TokenType type) {
        return &rules[type];
    }

    void parsePrecedence(Precedence precedence) {
        advance();
        ParseFn prefixRule = getRule(previous.type)->prefix;
        if (prefixRule == NULL) {
            error("Expect expression.");
            return;
        }

        bool canAssign = precedence <= PREC_ASSIGNMENT;
        (this->*prefixRule)(canAssign);

        while (precedence <= getRule(current.type)->precedence) {
            advance();
            ParseFn infixRule = getRule(previous.type)->infix;
            (this->*infixRule)(canAssign);
        }

        if (canAssign && match(TOKEN_EQUAL)) {
            error("Invalid assignment target.");
        }
    }

    uint8_t parseVariable(const std::string& errorMessage) {
        consume(TOKEN_IDENTIFIER, errorMessage);

        declareVariable();
        if (compiler.scopeDepth > 0) {
            return 0;
        }

        return identifierConstant(&previous);
    }

    void markInitialized() {
        compiler.locals[compiler.locals.size() - 1].depth = compiler.scopeDepth;
    }

    uint8_t identifierConstant(Token* name) {
        ObjString* value = new ObjString(std::string(previous.start, previous.end), objects);
        objects = (Obj*)value;
        return makeConstant((Obj*)value);
    }

    void addLocal(Token name) {
        if (compiler.locals.size() == 256) {
            error("Too many local variables in function.");
            return;
        }

        compiler.locals.push_back(Local(name, -1));
    }

    void declareVariable() {
        if (compiler.scopeDepth == 0) {
            return;
        }

        for (int i = compiler.locals.size() - 1; i >= 0; i--) {
            Local& local = compiler.locals[i];
            if (local.depth != -1 && local.depth < compiler.scopeDepth) {
                break;
            }

            if (previous.start == local.name.start) {
                error("Already a variable with this name in this scope.");
            }
        }

        addLocal(previous);
    }

    int resolveLocal(Token* name) {
        for (int i = compiler.locals.size() - 1; i >= 0; i--) {
            Local& local = compiler.locals[i];

            if (std::string(name->start, name->end) == std::string(local.name.start, local.name.end)) {
                if (local.depth == -1) {
                    error("Can't read local variable in its own initializer.");
                }
                return i;
            }
        }

        return -1;
    }

    void defineVariable(uint8_t global) {
        if (compiler.scopeDepth > 0) {
            markInitialized();
            return;
        }

        emitBytes(OP_DEFINE_GLOBAL, global);
    }

    void and_(bool canAssign) {
        int endJump = emitJump(OP_JUMP_IF_FALSE);

        emitByte(OP_POP);
        parsePrecedence(PREC_AND);

        patchJump(endJump);
    }

public:
    Parser(const std::string& source, Compiler& compiler, Obj* objects1) : compiler(compiler) {
        current_char = source.begin();
        end_char = source.end();
        objects = objects1;
    }

    ObjFunction* compile() {
        advance();

        while (!match(TOKEN_EOF)) {
            declaration();
        }

        consume(TOKEN_EOF, "Expect end of expression.");
        emitByte(OP_RETURN);
        return hadError ? nullptr : compiler.function;
    }
};

ObjFunction* compile(const std::string& source, Obj* objects) {
    Compiler compiler;
    compiler.function = new ObjFunction(objects);

    std::string name = "";
    Token token(TOKEN_IDENTIFIER, name.begin(), name.end(), 0);
    compiler.locals.push_back(Local(token, 0));

    Parser parser(source, compiler, objects);
    return parser.compile();
}