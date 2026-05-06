#include "Token.hpp"
#include <unordered_map>

namespace cnlab {

static const std::unordered_map<TokenType, std::string> tokenNames = {
    {TokenType::NUMBER, "NUMBER"},
    {TokenType::STRING, "STRING"},
    {TokenType::IDENTIFIER, "IDENTIFIER"},
    {TokenType::PLUS, "PLUS"},
    {TokenType::MINUS, "MINUS"},
    {TokenType::STAR, "STAR"},
    {TokenType::SLASH, "SLASH"},
    {TokenType::CARET, "CARET"},
    {TokenType::TRANSPOSE, "TRANSPOSE"},
    {TokenType::ASSIGN, "ASSIGN"},
    {TokenType::EQUAL, "EQUAL"},
    {TokenType::NOT_EQUAL, "NOT_EQUAL"},
    {TokenType::LESS, "LESS"},
    {TokenType::GREATER, "GREATER"},
    {TokenType::LESS_EQUAL, "LESS_EQUAL"},
    {TokenType::GREATER_EQUAL, "GREATER_EQUAL"},
    {TokenType::AND, "AND"},
    {TokenType::OR, "OR"},
    {TokenType::NOT, "NOT"},
    {TokenType::LPAREN, "LPAREN"},
    {TokenType::RPAREN, "RPAREN"},
    {TokenType::LBRACKET, "LBRACKET"},
    {TokenType::RBRACKET, "RBRACKET"},
    {TokenType::LBRACE, "LBRACE"},
    {TokenType::RBRACE, "RBRACE"},
    {TokenType::COMMA, "COMMA"},
    {TokenType::SEMICOLON, "SEMICOLON"},
    {TokenType::COLON, "COLON"},
    {TokenType::DOT, "DOT"},
    {TokenType::IF, "IF"},
    {TokenType::ELSE, "ELSE"},
    {TokenType::ELSEIF, "ELSEIF"},
    {TokenType::FOR, "FOR"},
    {TokenType::WHILE, "WHILE"},
    {TokenType::FUNCTION, "FUNCTION"},
    {TokenType::END, "END"},
    {TokenType::RETURN, "RETURN"},
    {TokenType::TRUE, "TRUE"},
    {TokenType::FALSE, "FALSE"},
    {TokenType::NEWLINE, "NEWLINE"},
    {TokenType::COMMENT, "COMMENT"},
    {TokenType::EOF_TOKEN, "EOF"},
    {TokenType::INVALID, "INVALID"}
};

std::string Token::toString() const {
    auto it = tokenNames.find(type);
    std::string name = (it != tokenNames.end()) ? it->second : "UNKNOWN";
    
    if (type == TokenType::NUMBER) {
        return name + "(" + std::to_string(std::get<double>(value)) + ")";
    }
    if (type == TokenType::STRING || type == TokenType::IDENTIFIER) {
        return name + "(" + std::get<std::string>(value) + ")";
    }
    if (type == TokenType::TRUE || type == TokenType::FALSE) {
        return name + "(" + std::string(std::get<bool>(value) ? "true" : "false") + ")";
    }
    return name;
}

}
