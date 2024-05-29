#ifndef scanner_h
#define scanner_h

#include <string>

enum TokenType {
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,
    TOKEN_ERROR, TOKEN_EOF
};

int tokenTypeToInt(TokenType type);

struct Token {
    TokenType type;
    std::string::const_iterator start;
    std::string::const_iterator end;
    int line;
};

class Scanner {
private:
    const std::string& source;
    std::string::const_iterator start;
    std::string::const_iterator current;
    int line = 1;

    bool isAtEnd();
    char advance();
    char peek();
    char peekNext();
    bool match(char expected);

    Token makeToken(TokenType type);
    Token errorToken(const std::string& message);
    void skipWhitespace();

    TokenType checkKeyword(int begin, std::string rest, TokenType type);
    TokenType identifierType();

    Token identifier();
    Token number();
    Token string();

public:
    Scanner(const std::string& source);
    Token scanToken();
};

#endif