#include "scanner.h"

const std::string UnexpectedCharacter = "Unexpected character.";
const std::string UnterminatedString = "Unterminated string.";

int tokenTypeToInt(TokenType type) {
    return static_cast<std::underlying_type<TokenType>::type>(type);
}

Scanner::Scanner(const std::string& source) : source(source) {
    start = source.begin();
    current = source.begin();
}

bool Scanner::isAtEnd() {
    return current == source.end() || *current == '\0';
}

char Scanner::advance() {
    current++;
    return current[-1];
}

char Scanner::peek() {
    return *current;
}

char Scanner::peekNext() {
    if (isAtEnd()) return '\0';
    return current[1];
}

bool Scanner::match(char expected) {
    if (isAtEnd()) return false;
    if (*current != expected) return false;
    current++;
    return true;
}

bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

Token Scanner::makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.start = start;
    token.end = current;
    token.line = line;
    return token;
}

Token Scanner::errorToken(const std::string& message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message.begin();
    token.end = message.end();
    token.line = line;
    return token;
}

void Scanner::skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            line++;
            advance();
            break;
        case '/':
            if (peekNext() == '/') {
                while (peek() != '\n' && !isAtEnd()) advance();
            } else {
                return;
            }
            break;
        default:
            return;
        }
    }
}

TokenType Scanner::checkKeyword(int begin, std::string rest, TokenType type) {
    if (current - start - begin != rest.size()) {
        return TOKEN_IDENTIFIER;
    }

    for (char c : rest) {
        if (start[begin] != c) {
            return TOKEN_IDENTIFIER;
        }
        begin++;
    }

    return type;
}

TokenType Scanner::identifierType() {
    switch (start[0]) {
    case 'a': return checkKeyword(1, "nd", TOKEN_AND);
    case 'c': return checkKeyword(1, "lass", TOKEN_CLASS);
    case 'e': return checkKeyword(1, "lse", TOKEN_ELSE);
    case 'f':
        if (current - start > 1) {
            switch (start[1]) {
            case 'a': return checkKeyword(2, "lse", TOKEN_FALSE);
            case 'o': return checkKeyword(2, "r", TOKEN_FOR);
            case 'u': return checkKeyword(2, "n", TOKEN_FUN);
            }
        }
        break;
    case 'i': return checkKeyword(1, "f", TOKEN_IF);
    case 'n': return checkKeyword(1, "il", TOKEN_NIL);
    case 'o': return checkKeyword(1, "r", TOKEN_OR);
    case 'p': return checkKeyword(1, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, "eturn", TOKEN_RETURN);
    case 's': return checkKeyword(1, "uper", TOKEN_SUPER);
    case 't':
        if (current - start > 1) {
            switch (start[1]) {
            case 'h': return checkKeyword(2, "is", TOKEN_THIS);
            case 'r': return checkKeyword(2, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'v': return checkKeyword(1, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(1, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

Token Scanner::identifier() {
    while (isAlpha(peek()) || isDigit(peek())) advance();
    return makeToken(identifierType());
}

Token Scanner::number() {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) advance();
    }

    return makeToken(TOKEN_NUMBER);
}

Token Scanner::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line++;
        advance();
    }

    if (isAtEnd()) return errorToken(UnterminatedString);

    advance();
    return makeToken(TOKEN_STRING);
}

Token Scanner::scanToken() {
    skipWhitespace();

    start = current;
    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }

    char c = advance();
    if (isAlpha(c)) return identifier();
    if (isDigit(c)) return number();

    switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': return makeToken(TOKEN_DOT);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(TOKEN_STAR);
    case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>': return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
    }

    return errorToken(UnexpectedCharacter);
}