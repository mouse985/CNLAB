#pragma once

#include "Token.hpp"
#include "AST.hpp"
#include <vector>
#include <memory>
#include <stdexcept>

namespace cnlab {

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    
    std::shared_ptr<Program> parse();
    
private:
    std::vector<Token> tokens_;
    size_t current_;
    
    std::shared_ptr<Statement> parseStatement();
    std::shared_ptr<Statement> parseExpressionStatement();
    std::shared_ptr<Statement> parseAssignmentOrExpression();
    std::shared_ptr<Statement> parseIfStatement();
    std::shared_ptr<Statement> parseWhileStatement();
    std::shared_ptr<Statement> parseForStatement();
    std::shared_ptr<Statement> parseSwitchStatement();
    std::shared_ptr<Statement> parseTryCatchStatement();
    std::shared_ptr<Statement> parseFunctionDeclaration();
    std::shared_ptr<Statement> parseReturnStatement();
    std::shared_ptr<Statement> parseBreakStatement();
    std::shared_ptr<Statement> parseContinueStatement();
    std::shared_ptr<Statement> parseGlobalStatement();
    std::shared_ptr<Statement> parseClearCommand();
    
    ExprPtr parseExpression();
    ExprPtr parseRange();
    ExprPtr parseOr();
    ExprPtr parseIndexExpression();
    ExprPtr parseAnd();
    ExprPtr parseElementOr();
    ExprPtr parseElementAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAdditive();
    ExprPtr parseMultiplicative();
    ExprPtr parsePower();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
    ExprPtr parseMatrixLiteral();
    ExprPtr parseCellLiteral();
    
    bool match(std::initializer_list<TokenType> types);
    bool check(TokenType type) const;
    bool isAtEnd() const;
    Token advance();
    Token peek() const;
    Token previous() const;
    Token consume(TokenType type, const std::string& message);
    bool canStartExpression(TokenType type) const;
    
    void synchronize();
    ParseError error(const Token& token, const std::string& message);
};

}
