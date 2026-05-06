#pragma once

#include "Token.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace cnlab {

class Lexer {
public:
    explicit Lexer(const std::string& source);
    
    std::vector<Token> scanTokens();
    
private:
    std::string source_;
    std::vector<Token> tokens_;
    size_t start_;
    size_t current_;
    size_t line_;
    size_t column_;
    
    static const std::unordered_map<std::string, TokenType> keywords_;
    
    void scanToken();
    void addToken(TokenType type);
    void addToken(TokenType type, double value);
    void addToken(TokenType type, const std::string& value);
    void addToken(TokenType type, bool value);
    
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    char peekNextNext() const;
    bool match(char expected);
    
    void string();
    void charArray();
    void number();
    void identifier();

    void skipWhitespace();
    void skipComment();
    void skipMultiLineComment();
    
    static bool isDigit(char c);
    static bool isAlpha(char c);
    static bool isAlphaNumeric(char c);
};

}
