#pragma once

#include <string>
#include <variant>

namespace cnlab {

enum class TokenType {
    // Literals
    NUMBER,
    COMPLEX,
    STRING,
    CHAR_ARRAY,

    // Identifiers
    IDENTIFIER,
    
    // Operators
    PLUS,
    MINUS,
    STAR,
    SLASH,
    MODULO,
    CARET,
    TRANSPOSE,
    ASSIGN,
    
    // Comparison
    EQUAL,
    NOT_EQUAL,
    LESS,
    GREATER,
    LESS_EQUAL,
    GREATER_EQUAL,
    
    // Logical
    AND,
    OR,
    NOT,
    TILDE,
    
    // Element-wise logical
    ELEMENT_AND,
    ELEMENT_OR,
    
    // Element-wise arithmetic
    ELEMENT_MUL,
    ELEMENT_DIV,
    ELEMENT_POW,
    
    // Left division (backslash)
    BACKSLASH,
    
    // Indexing
    END_INDEX,
    
    // Transpose
    TRANSPOSE_NON_CONJ,
    
    // Delimiters
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    COMMA,
    SEMICOLON,
    COLON,
    DOT,
    
    // Keywords
    IF,
    ELSE,
    ELSEIF,
    FOR,
    WHILE,
    FUNCTION,
    END,
    RETURN,
    TRUE,
    FALSE,
    BREAK,
    CONTINUE,
    SWITCH,
    CASE,
    OTHERWISE,
    TRY,
    CATCH,
    GLOBAL,
    VARARGIN,
    VARARGOUT,
    NARGIN,
    NARGOUT,
    STRUCT,
    CELL,
    TABLE,
    CATEGORICAL,
    DATETIME,
    DURATION,
    
    // Special
    AT,
    NEWLINE,
    COMMENT,
    EOF_TOKEN,
    INVALID
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::variant<double, std::string, bool> value;
    size_t line;
    size_t column;
    
    Token(TokenType t, const std::string& l, size_t ln, size_t col)
        : type(t), lexeme(l), line(ln), column(col) {}
    
    Token(TokenType t, const std::string& l, double v, size_t ln, size_t col)
        : type(t), lexeme(l), value(v), line(ln), column(col) {}
    
    Token(TokenType t, const std::string& l, const std::string& v, size_t ln, size_t col)
        : type(t), lexeme(l), value(v), line(ln), column(col) {}
    
    Token(TokenType t, const std::string& l, bool v, size_t ln, size_t col)
        : type(t), lexeme(l), value(v), line(ln), column(col) {}
    
    bool is(TokenType t) const { return type == t; }
    
    std::string toString() const;
};

}
