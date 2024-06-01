#include "scanner.h"

const std::string UnexpectedCharacter = "Unexpected character.";
const std::string UnterminatedString = "Unterminated string.";
const std::string UnterminatedComment = "Unterminated comment.";

Token::Token() {}

Token::Token(TokenType type1, StringIterator start1, StringIterator end1, int line1) {
    type = type1;
    start = start1;
    end = end1;
    line = line1;
}

bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

TokenType checkKeyword(StringIterator begin, StringIterator end, std::string rest, TokenType type) {
    if (end - begin != rest.size()) {
        return TOKEN_IDENTIFIER;
    }

    for (char c : rest) {
        if (begin[0] != c) {
            return TOKEN_IDENTIFIER;
        }
        begin++;
    }

    return type;
}

TokenType identifierType(StringIterator start, StringIterator end) {
    switch (start[0]) {
    case 'a': return checkKeyword(start + 1, end, "nd", TOKEN_AND);
    case 'c': return checkKeyword(start + 1, end, "lass", TOKEN_CLASS);
    case 'e': return checkKeyword(start + 1, end, "lse", TOKEN_ELSE);
    case 'f':
        if (start + 1 != end) {
            switch (start[1]) {
            case 'a': return checkKeyword(start + 2, end, "lse", TOKEN_FALSE);
            case 'o': return checkKeyword(start + 2, end, "r", TOKEN_FOR);
            case 'u': return checkKeyword(start + 2, end, "n", TOKEN_FUN);
            }
        }
        break;
    case 'i': return checkKeyword(start + 1, end, "f", TOKEN_IF);
    case 'n': return checkKeyword(start + 1, end, "il", TOKEN_NIL);
    case 'o': return checkKeyword(start + 1, end, "r", TOKEN_OR);
    case 'p': return checkKeyword(start + 1, end, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(start + 1, end, "eturn", TOKEN_RETURN);
    case 's': return checkKeyword(start + 1, end, "uper", TOKEN_SUPER);
    case 't':
        if (start + 1 != end) {
            switch (start[1]) {
            case 'h': return checkKeyword(start + 2, end, "is", TOKEN_THIS);
            case 'r': return checkKeyword(start + 2, end, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'v': return checkKeyword(start + 1, end, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(start + 1, end, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

Token identifier(StringIterator& current, StringIterator end, int line) {
    StringIterator start = current;
    while (isAlpha(current[0]) || isDigit(current[0])) current++;
    return Token(identifierType(start, current), start, current, line);
}

Token number(StringIterator& current, StringIterator end, int line) {
    StringIterator start = current;

    while (isDigit(current[0])) current++;

    if (current[0] == '.' && current + 1 != end && isDigit(current[1])) {
        current++;
        while (isDigit(current[0])) current++;
    }

    return Token(TOKEN_NUMBER, start, current, line);
}

Token string(StringIterator& current, StringIterator end, int& line) {
    StringIterator start = current;
    current++;

    while (current != end && current[0] != '"') {
        if (current[0] == '\n') line++;
        current++;
    }

    if (current == end) {
        return Token(TOKEN_ERROR, UnterminatedString.begin(), UnterminatedString.end(), line);
    }

    current++;
    return Token(TOKEN_STRING, start, current, line);
}

Token character(StringIterator& current, StringIterator end, int line) {
    StringIterator start = current;
    current++;

    switch (*start) {
    case '(': return Token(TOKEN_LEFT_PAREN, start, current, line);
    case ')': return Token(TOKEN_RIGHT_PAREN, start, current, line);
    case '{': return Token(TOKEN_LEFT_BRACE, start, current, line);
    case '}': return Token(TOKEN_RIGHT_BRACE, start, current, line);
    case ';': return Token(TOKEN_SEMICOLON, start, current, line);
    case ',': return Token(TOKEN_COMMA, start, current, line);
    case '.': return Token(TOKEN_DOT, start, current, line);
    case '-': return Token(TOKEN_MINUS, start, current, line);
    case '+': return Token(TOKEN_PLUS, start, current, line);
    case '/': return Token(TOKEN_SLASH, start, current, line);
    case '*': return Token(TOKEN_STAR, start, current, line);
    case '!':
        if (current != end && current[0] == '=') {
            current++;
            return Token(TOKEN_BANG_EQUAL, start, current, line);
        } else return Token(TOKEN_BANG, start, current, line);
    case '=':
        if (current != end && current[0] == '=') {
            current++;
            return Token(TOKEN_EQUAL_EQUAL, start, current, line);
        } else return Token(TOKEN_EQUAL, start, current, line);
    case '<':
        if (current != end && current[0] == '=') {
            current++;
            return Token(TOKEN_LESS_EQUAL, start, current, line);
        } else return Token(TOKEN_LESS, start, current, line);
    case '>':
        if (current != end && current[0] == '=') {
            current++;
            return Token(TOKEN_GREATER_EQUAL, start, current, line);
        } else return Token(TOKEN_GREATER, start, current, line);
    }

    return Token(TOKEN_ERROR, UnexpectedCharacter.begin(), UnexpectedCharacter.end(), line);
}

Token scanToken(StringIterator& current, StringIterator end, int& line) {
    while (current != end) {
        if (current[0] == ' ' || current[0] == '\r' || current[0] == '\t') {
            current++;
            continue;
        } else if (current[0] == '\n') {
            current++;
            line++;
            continue;
        }

        if (current[0] == '/' && current + 1 != end) {
            if (current[1] == '/') {
                current += 2;
                while (current != end && current[0] != '\n') current++;
                continue;
            }

            if (current[1] == '*') {
                current += 2;

                if (current == end || current + 1 == end) {
                    return Token(TOKEN_ERROR, UnterminatedComment.begin(), UnterminatedComment.end(), line);
                }

                while (current + 1 != end && current[0] != '*' && current[1] != '/') current++;

                if (current[0] != '*' && current[1] != '/') {
                    return Token(TOKEN_ERROR, UnterminatedComment.begin(), UnterminatedComment.end(), line);
                }

                current += 2;
                continue;
            }
        }

        if (isAlpha(current[0])) {
            return identifier(current, end, line);
        }

        if (isDigit(current[0])) {
            return number(current, end, line);
        }

        if (current[0] == '"') {
            return string(current, end, line);
        }

        return character(current, end, line);
    }

    return Token(TOKEN_EOF, current, end, line);
}