#include "Parser.hpp"
#include <iostream>

namespace cnlab {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), current_(0) {}

std::shared_ptr<Program> Parser::parse() {
    auto program = std::make_shared<Program>();
    
    while (!isAtEnd()) {
        try {
            // Skip empty lines, commas, and semicolons at the start of a statement
            while (match({TokenType::NEWLINE}) || match({TokenType::COMMA}) || match({TokenType::SEMICOLON})) {
                // Just skip them
            }
            
            if (isAtEnd()) break;
            
            auto stmt = parseStatement();
            if (stmt) {
                program->statements.push_back(stmt);
            }
            
            // MATLAB-compatible: support comma as statement separator
            // After a statement, we can have:
            // 1. Newline (end of statement)
            // 2. Semicolon (end of statement, suppress output)
            // 3. Comma (continue to next statement on same line)
            // 4. End of file
            while (!isAtEnd() && !check(TokenType::NEWLINE)) {
                if (match({TokenType::COMMA})) {
                    // Comma separates statements, continue to parse next statement
                    // But skip any additional commas, semicolons or newlines
                    while (match({TokenType::COMMA}) || match({TokenType::SEMICOLON}) || match({TokenType::NEWLINE})) {
                        // Skip
                    }
                    
                    if (!isAtEnd()) {
                        auto nextStmt = parseStatement();
                        if (nextStmt) {
                            program->statements.push_back(nextStmt);
                        }
                    }
                } else if (match({TokenType::SEMICOLON})) {
                    // Semicolon ends statement, skip it and continue
                    continue;
                } else if (check(TokenType::END) || 
                           check(TokenType::ELSE) || check(TokenType::ELSEIF) ||
                           check(TokenType::CATCH) || check(TokenType::OTHERWISE)) {
                    // These tokens indicate end of current statement/scope
                    break;
                } else {
                    // Unexpected token, break to avoid infinite loop
                    break;
                }
            }
        } catch (const ParseError& e) {
            std::cerr << "Parse error: " << e.what() << std::endl;
            synchronize();
        }
    }
    
    return program;
}

std::shared_ptr<Statement> Parser::parseStatement() {
	if (match({TokenType::IF})) return parseIfStatement();
	if (match({TokenType::WHILE})) return parseWhileStatement();
	if (match({TokenType::FOR})) return parseForStatement();
	if (match({TokenType::SWITCH})) return parseSwitchStatement();
	if (match({TokenType::TRY})) return parseTryCatchStatement();
	if (match({TokenType::FUNCTION})) return parseFunctionDeclaration();
	if (match({TokenType::RETURN})) return parseReturnStatement();
	if (match({TokenType::BREAK})) return parseBreakStatement();
	if (match({TokenType::CONTINUE})) return parseContinueStatement();
	if (match({TokenType::GLOBAL})) return parseGlobalStatement();
	if (match({TokenType::NEWLINE})) return nullptr;
	
	// Handle command-style function calls like "clear x y z"
	if (check(TokenType::IDENTIFIER)) {
		Token id = peek();
		std::string name = std::get<std::string>(id.value);
		if (name == "clear") {
			return parseClearCommand();
		}
	}
	
	return parseAssignmentOrExpression();
}

std::shared_ptr<Statement> Parser::parseClearCommand() {
	// Consume "clear"
	advance();
	
	std::vector<ExprPtr> args;
	
	// Parse variable names until end of statement
	while (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !isAtEnd()) {
		if (check(TokenType::IDENTIFIER)) {
			Token var = advance();
			std::string varName = std::get<std::string>(var.value);
			args.push_back(std::make_shared<StringLiteral>(varName));
		} else if (check(TokenType::STRING)) {
			Token str = advance();
			args.push_back(std::make_shared<StringLiteral>(std::get<std::string>(str.value)));
		} else {
			break;
		}
	}
	
	// Create a call expression for clear
	return std::make_shared<ExpressionStatement>(
		std::make_shared<CallExpression>(
			std::make_shared<Identifier>("clear"),
			args
		)
	);
}

std::shared_ptr<Statement> Parser::parseAssignmentOrExpression() {
    // Check for multi-assignment: [a, b] = expr or [a, ~, b] = expr
    if (check(TokenType::LBRACKET)) {
        size_t savePos = current_;
        advance(); // consume '['
        
        std::vector<std::string> names;
        std::vector<bool> isIgnored;
        bool isMultiAssign = false;
        
        // Parse variable list or matrix literal
        if (!check(TokenType::RBRACKET)) {
            do {
                if (match({TokenType::TILDE})) {
                    names.push_back("");
                    isIgnored.push_back(true);
                    isMultiAssign = true;
                } else if (check(TokenType::IDENTIFIER)) {
                    Token var = advance();
                    names.push_back(std::get<std::string>(var.value));
                    isIgnored.push_back(false);
                    isMultiAssign = true;
                } else {
                    // Not a multi-assignment, backtrack
                    current_ = savePos;
                    break;
                }
            } while (match({TokenType::COMMA}));
        }
        
        if (isMultiAssign && check(TokenType::RBRACKET) && !check(TokenType::COMMA)) {
            // Check next token after ']' to see if this is assignment
            size_t afterBracket = current_ + 1;
            if (afterBracket < tokens_.size() && tokens_[afterBracket].type == TokenType::ASSIGN) {
                advance(); // consume ']'
                advance(); // consume '='
                ExprPtr value = parseExpression();
                return std::make_shared<MultiAssignmentStatement>(std::move(names), std::move(isIgnored), value);
            }
        }
        
        // Not a multi-assignment, backtrack and parse as expression
        current_ = savePos;
    }
    
    ExprPtr expr = parseExpression();
    
    if (auto* id = dynamic_cast<Identifier*>(expr.get())) {
        if (match({TokenType::ASSIGN})) {
            ExprPtr value = parseExpression();
            return std::make_shared<AssignmentStatement>(id->name, value);
        }
    }
    
    if (auto* idxExpr = dynamic_cast<IndexExpression*>(expr.get())) {
        if (match({TokenType::ASSIGN})) {
            ExprPtr value = parseExpression();
            if (auto* id = dynamic_cast<Identifier*>(idxExpr->object.get())) {
                return std::make_shared<IndexAssignmentStatement>(id->name, idxExpr->indices, value);
            }
        }
    }
    
    // Handle cell index assignment: C{1,2} = value
    if (auto* cellIdxExpr = dynamic_cast<CellIndexExpression*>(expr.get())) {
        if (match({TokenType::ASSIGN})) {
            ExprPtr value = parseExpression();
            if (auto* id = dynamic_cast<Identifier*>(cellIdxExpr->cell.get())) {
                return std::make_shared<CellIndexAssignmentStatement>(id->name, cellIdxExpr->indices, value);
            }
        }
    }
    
    // Handle field assignment: s.field = value
    if (auto* fieldExpr = dynamic_cast<FieldAccessExpression*>(expr.get())) {
        if (match({TokenType::ASSIGN})) {
            ExprPtr value = parseExpression();
            return std::make_shared<FieldAssignmentStatement>(fieldExpr->object, fieldExpr->fieldName, value);
        }
    }
    
    return std::make_shared<ExpressionStatement>(expr);
}

std::shared_ptr<Statement> Parser::parseIfStatement() {
    auto stmt = std::make_shared<IfStatement>();
    
    stmt->condition = parseExpression();
    
    while (!check(TokenType::ELSE) && !check(TokenType::ELSEIF) && 
           !check(TokenType::END) && !isAtEnd()) {
        auto bodyStmt = parseStatement();
        if (bodyStmt) stmt->thenBranch.push_back(bodyStmt);
    }
    
    while (match({TokenType::ELSEIF})) {
        ExprPtr elseifCond = parseExpression();
        std::vector<StmtPtr> elseifBody;
        
        while (!check(TokenType::ELSE) && !check(TokenType::ELSEIF) && 
               !check(TokenType::END) && !isAtEnd()) {
            auto bodyStmt = parseStatement();
            if (bodyStmt) elseifBody.push_back(bodyStmt);
        }
        
        stmt->elseifBranches.emplace_back(elseifCond, elseifBody);
    }
    
    if (match({TokenType::ELSE})) {
        while (!check(TokenType::END) && !isAtEnd()) {
            auto bodyStmt = parseStatement();
            if (bodyStmt) stmt->elseBranch.push_back(bodyStmt);
        }
    }
    
    consume(TokenType::END, "Expected 'end' after if statement");
    return stmt;
}

std::shared_ptr<Statement> Parser::parseWhileStatement() {
    auto stmt = std::make_shared<WhileStatement>();
    
    stmt->condition = parseExpression();
    
    while (!check(TokenType::END) && !isAtEnd()) {
        auto bodyStmt = parseStatement();
        if (bodyStmt) stmt->body.push_back(bodyStmt);
    }
    
    consume(TokenType::END, "Expected 'end' after while statement");
    return stmt;
}

std::shared_ptr<Statement> Parser::parseForStatement() {
    Token var = consume(TokenType::IDENTIFIER, "Expected variable name after 'for'");
    consume(TokenType::ASSIGN, "Expected '=' after for variable");
    
    auto stmt = std::make_shared<ForStatement>();
    stmt->variable = std::get<std::string>(var.value);
    stmt->range = parseExpression();
    
    while (!check(TokenType::END) && !isAtEnd()) {
        auto bodyStmt = parseStatement();
        if (bodyStmt) stmt->body.push_back(bodyStmt);
    }
    
    consume(TokenType::END, "Expected 'end' after for statement");
    return stmt;
}

std::shared_ptr<Statement> Parser::parseSwitchStatement() {
    auto stmt = std::make_shared<SwitchStatement>();
    
    // Parse switch expression
    stmt->expression = parseExpression();
    
    // Consume newlines after switch expression
    while (match({TokenType::NEWLINE})) {}
    
    // Parse case clauses
    while (match({TokenType::CASE})) {
        CaseClause caseClause;
        
        // Parse case values (can be multiple: case {1, 2, 3})
        if (match({TokenType::LBRACE})) {
            // Multiple values: case {1, 2, 3}
            do {
                caseClause.values.push_back(parseExpression());
            } while (match({TokenType::COMMA}));
            consume(TokenType::RBRACE, "Expected '}' after case values");
        } else {
            // Single value
            caseClause.values.push_back(parseExpression());
        }
        
        // Consume newlines after case value
        while (match({TokenType::NEWLINE})) {}
        
        // Parse case body (until next case, otherwise, or end)
        while (!check(TokenType::CASE) && !check(TokenType::OTHERWISE) && 
               !check(TokenType::END) && !isAtEnd()) {
            auto bodyStmt = parseStatement();
            if (bodyStmt) caseClause.body.push_back(bodyStmt);
        }
        
        stmt->cases.push_back(std::move(caseClause));
    }
    
    // Parse otherwise branch
    if (match({TokenType::OTHERWISE})) {
        // Consume newlines after otherwise
        while (match({TokenType::NEWLINE})) {}
        
        while (!check(TokenType::END) && !isAtEnd()) {
            auto bodyStmt = parseStatement();
            if (bodyStmt) stmt->otherwiseBranch.push_back(bodyStmt);
        }
    }
    
    consume(TokenType::END, "Expected 'end' after switch statement");
    return stmt;
}

std::shared_ptr<Statement> Parser::parseTryCatchStatement() {
    auto stmt = std::make_shared<TryCatchStatement>();
    
    // Consume newlines after try
    while (match({TokenType::NEWLINE})) {}
    
    // Parse try body
    while (!check(TokenType::CATCH) && !check(TokenType::END) && !isAtEnd()) {
        auto bodyStmt = parseStatement();
        if (bodyStmt) stmt->tryBody.push_back(bodyStmt);
    }
    
    // Parse catch block
    if (match({TokenType::CATCH})) {
        // Optional exception variable
        if (check(TokenType::IDENTIFIER)) {
            Token var = advance();
            stmt->exceptionVar = std::get<std::string>(var.value);
        }
        
        // Consume newlines after catch/catch ME
        while (match({TokenType::NEWLINE})) {}
        
        // Parse catch body
        while (!check(TokenType::END) && !isAtEnd()) {
            auto bodyStmt = parseStatement();
            if (bodyStmt) stmt->catchBody.push_back(bodyStmt);
        }
    }
    
    consume(TokenType::END, "Expected 'end' after try-catch statement");
    return stmt;
}

std::shared_ptr<Statement> Parser::parseFunctionDeclaration() {
    // MATLAB style: function y = square(x) or function square(x)
    auto stmt = std::make_shared<FunctionDeclaration>();
    
    // Parse optional output variable (e.g., 'y' in 'function y = square(x)')
    std::string outputVar;
    if (check(TokenType::IDENTIFIER)) {
        Token firstId = advance();
        if (match({TokenType::ASSIGN})) {
            // This is output variable, e.g., 'y ='
            outputVar = std::get<std::string>(firstId.value);
        } else {
            // No output variable, this is the function name
            // Backtrack and treat as function name
            current_--;
        }
    }
    
    // Parse function name
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    stmt->name = std::get<std::string>(name.value);
    
    // If there was an output variable, add it as a special parameter or handle in body
    if (!outputVar.empty()) {
        // Store output variable name for use in function body
        // We'll add it as the first parameter internally
        stmt->parameters.push_back("__output__" + outputVar);
    }
    
    if (match({TokenType::LPAREN})) {
        if (!check(TokenType::RPAREN)) {
            do {
                if (match({TokenType::VARARGIN})) {
                    stmt->hasVarargin = true;
                    stmt->parameters.push_back("varargin");
                } else {
                    Token param = consume(TokenType::IDENTIFIER, "Expected parameter name");
                    stmt->parameters.push_back(std::get<std::string>(param.value));
                }
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RPAREN, "Expected ')' after parameters");
    }
    
    while (!check(TokenType::END) && !isAtEnd()) {
        auto bodyStmt = parseStatement();
        if (bodyStmt) stmt->body.push_back(bodyStmt);
    }
    
    consume(TokenType::END, "Expected 'end' after function declaration");
    return stmt;
}

std::shared_ptr<Statement> Parser::parseReturnStatement() {
	ExprPtr value = nullptr;
	if (!check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON) && !isAtEnd()) {
		value = parseExpression();
	}
	return std::make_shared<ReturnStatement>(value);
}

std::shared_ptr<Statement> Parser::parseBreakStatement() {
	return std::make_shared<BreakStatement>();
}

std::shared_ptr<Statement> Parser::parseContinueStatement() {
	return std::make_shared<ContinueStatement>();
}

std::shared_ptr<Statement> Parser::parseGlobalStatement() {
    std::vector<std::string> vars;
    
    // Parse at least one variable name
    do {
        Token var = consume(TokenType::IDENTIFIER, "Expected variable name after 'global'");
        vars.push_back(std::get<std::string>(var.value));
    } while (check(TokenType::IDENTIFIER));
    
    return std::make_shared<GlobalStatement>(std::move(vars));
}

ExprPtr Parser::parseExpression() {
    return parseRange();
}

ExprPtr Parser::parseIndexExpression() {
    // Parse the start expression
    ExprPtr start = parseOr();
    
    // Check if this is a range expression (start:end or start:step:end)
    if (match({TokenType::COLON})) {
        ExprPtr step = nullptr;
        ExprPtr end = nullptr;
        
        // Check if there's a step (start:step:end)
        ExprPtr potentialStepOrEnd = parseOr();
        
        if (match({TokenType::COLON})) {
            // This is start:step:end
            step = potentialStepOrEnd;
            end = parseOr();
        } else {
            // This is start:end (step defaults to 1)
            step = std::make_shared<NumberLiteral>(1.0);
            end = potentialStepOrEnd;
        }
        
        return std::make_shared<RangeExpression>(start, step, end);
    }
    
    return start;
}

ExprPtr Parser::parseRange() {
    ExprPtr expr = parseOr();
    
    if (match({TokenType::COLON})) {
        ExprPtr step = nullptr;
        ExprPtr end = nullptr;
        
        // Parse the next expression - could be end (in start:end) or step (in start:step:end)
        ExprPtr nextExpr = parseOr();
        
        if (match({TokenType::COLON})) {
            // This is start:step:end format
            step = nextExpr;
            end = parseOr();
        } else {
            // This is start:end format, step defaults to 1
            step = std::make_shared<NumberLiteral>(1.0);
            end = nextExpr;
        }
        
        return std::make_shared<RangeExpression>(expr, step, end);
    }
    
    return expr;
}

ExprPtr Parser::parseOr() {
    ExprPtr expr = parseAnd();
    
    while (match({TokenType::OR})) {
        std::string op = "||";
        ExprPtr right = parseAnd();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parseAnd() {
    ExprPtr expr = parseElementOr();
    
    while (match({TokenType::AND})) {
        std::string op = "&&";
        ExprPtr right = parseElementOr();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parseElementOr() {
    ExprPtr expr = parseElementAnd();
    
    while (match({TokenType::ELEMENT_OR})) {
        std::string op = "|";
        ExprPtr right = parseElementAnd();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parseElementAnd() {
    ExprPtr expr = parseEquality();
    
    while (match({TokenType::ELEMENT_AND})) {
        std::string op = "&";
        ExprPtr right = parseEquality();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseComparison();
    
    while (match({TokenType::EQUAL, TokenType::NOT_EQUAL})) {
        std::string op = previous().lexeme;
        ExprPtr right = parseComparison();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parseComparison() {
    ExprPtr expr = parseAdditive();
    
    while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, 
                   TokenType::LESS, TokenType::LESS_EQUAL})) {
        std::string op = previous().lexeme;
        ExprPtr right = parseAdditive();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parseAdditive() {
    ExprPtr expr = parseMultiplicative();
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        std::string op = previous().lexeme;
        ExprPtr right = parseMultiplicative();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parseMultiplicative() {
    ExprPtr expr = parsePower();
    
    while (match({TokenType::STAR, TokenType::SLASH, TokenType::MODULO,
                  TokenType::ELEMENT_MUL, TokenType::ELEMENT_DIV, TokenType::BACKSLASH})) {
        std::string op = previous().lexeme;
        ExprPtr right = parsePower();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parsePower() {
    ExprPtr expr = parseUnary();
    
    if (match({TokenType::CARET, TokenType::ELEMENT_POW})) {
        std::string op = previous().lexeme;
        ExprPtr right = parsePower();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    
    return expr;
}

ExprPtr Parser::parseUnary() {
    if (match({TokenType::MINUS, TokenType::NOT, TokenType::PLUS})) {
        std::string op = previous().lexeme;
        ExprPtr operand = parseUnary();
        return std::make_shared<UnaryExpression>(op, operand);
    }
    
    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    ExprPtr expr = parsePrimary();

    while (true) {
        if (match({TokenType::LPAREN})) {
            std::vector<ExprPtr> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    if (match({TokenType::COLON})) {
                        args.push_back(std::make_shared<ColonExpression>());
                    } else {
                        args.push_back(parseIndexExpression());
                    }
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expected ')' after arguments");
            expr = std::make_shared<IndexExpression>(expr, args);
        } else if (match({TokenType::LBRACKET})) {
            std::vector<ExprPtr> indices;
            do {
                if (match({TokenType::COLON})) {
                    indices.push_back(std::make_shared<ColonExpression>());
                } else {
                    indices.push_back(parseIndexExpression());
                }
            } while (match({TokenType::COMMA}));
            consume(TokenType::RBRACKET, "Expected ']' after indices");
            expr = std::make_shared<IndexExpression>(expr, indices);
        } else if (match({TokenType::LBRACE})) {
            std::vector<ExprPtr> indices;
            do {
                if (match({TokenType::COLON})) {
                    indices.push_back(std::make_shared<ColonExpression>());
                } else {
                    indices.push_back(parseIndexExpression());
                }
            } while (match({TokenType::COMMA}));
            consume(TokenType::RBRACE, "Expected '}' after indices");
            expr = std::make_shared<CellIndexExpression>(expr, indices);
        } else if (match({TokenType::TRANSPOSE})) {
            expr = std::make_shared<CallExpression>(
                std::make_shared<Identifier>("transpose"),
                std::vector<ExprPtr>{expr}
            );
        } else if (match({TokenType::TRANSPOSE_NON_CONJ})) {
            // Non-conjugate transpose (same as regular transpose for real matrices)
            expr = std::make_shared<CallExpression>(
                std::make_shared<Identifier>("transpose"),
                std::vector<ExprPtr>{expr}
            );
        } else if (match({TokenType::DOT})) {
            // Field access: s.field
            Token fieldName = consume(TokenType::IDENTIFIER, "Expected field name after '.'");
            expr = std::make_shared<FieldAccessExpression>(expr, std::get<std::string>(fieldName.value));
        } else {
            break;
        }
    }

    return expr;
}

ExprPtr Parser::parsePrimary() {
    if (match({TokenType::TRUE})) {
        return std::make_shared<BooleanLiteral>(true);
    }
    if (match({TokenType::FALSE})) {
        return std::make_shared<BooleanLiteral>(false);
    }
    if (match({TokenType::NUMBER})) {
        return std::make_shared<NumberLiteral>(std::get<double>(previous().value));
    }
    if (match({TokenType::COMPLEX})) {
        std::string complexStr = previous().lexeme;
        // Parse complex string like "3+4i" or "5i" or "1-2i"
        double real = 0.0, imag = 0.0;
        size_t iPos = complexStr.find('i');
        size_t jPos = complexStr.find('j');
        size_t imagPos = (iPos != std::string::npos) ? iPos : jPos;
        
        if (imagPos != std::string::npos) {
            std::string imagPart = complexStr.substr(0, imagPos);
            if (imagPart.empty() || imagPart == "+") {
                imag = 1.0;
            } else if (imagPart == "-") {
                imag = -1.0;
            } else {
                imag = std::stod(imagPart);
            }
        }
        return std::make_shared<ComplexLiteral>(real, imag);
    }
    if (match({TokenType::STRING})) {
        return std::make_shared<StringLiteral>(std::get<std::string>(previous().value));
    }
    if (match({TokenType::CHAR_ARRAY})) {
        return std::make_shared<CharArrayLiteral>(std::get<std::string>(previous().value));
    }
    if (match({TokenType::IDENTIFIER})) {
        return std::make_shared<Identifier>(std::get<std::string>(previous().value));
    }
    // Allow certain keywords to be used as identifiers (e.g., varargin, nargin, struct, etc.)
    if (check(TokenType::VARARGIN) || check(TokenType::NARGIN) || check(TokenType::NARGOUT) || check(TokenType::VARARGOUT) ||
        check(TokenType::STRUCT) || check(TokenType::CELL) || check(TokenType::TABLE) ||
        check(TokenType::CATEGORICAL) || check(TokenType::DATETIME) || check(TokenType::DURATION)) {
        Token keyword = advance();
        return std::make_shared<Identifier>(keyword.lexeme);
    }
    if (match({TokenType::LBRACE})) {
        return parseCellLiteral();
    }
    if (match({TokenType::LBRACKET})) {
        return parseMatrixLiteral();
    }
    if (match({TokenType::LPAREN})) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    if (match({TokenType::AT})) {
        // Anonymous function or function handle
        if (match({TokenType::LPAREN})) {
            // Anonymous function: @(params) expr
            std::vector<std::string> params;
            if (!check(TokenType::RPAREN)) {
                do {
                    Token param = consume(TokenType::IDENTIFIER, "Expected parameter name");
                    params.push_back(std::get<std::string>(param.value));
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expected ')' after parameters");
            ExprPtr body = parseExpression();
            return std::make_shared<AnonymousFunction>(std::move(params), std::move(body));
        } else if (check(TokenType::IDENTIFIER)) {
            // Function handle: @functionName
            Token name = advance();
            return std::make_shared<FunctionHandle>(std::get<std::string>(name.value));
        } else {
            throw error(peek(), "Expected '(' or function name after '@'");
        }
    }
    
    // Support 'end' as an expression (for indexing like A(end))
    if (match({TokenType::END})) {
        return std::make_shared<EndExpression>();
    }
    
    throw error(peek(), "Expected expression");
}

ExprPtr Parser::parseMatrixLiteral() {
    // Check if this is a string array: ["a", "b", "c"]
    // We need to look ahead to see if all elements are string literals
    size_t savePos = current_;
    bool allStrings = true;
    std::vector<ExprPtr> stringElements;
    
    if (!check(TokenType::RBRACKET)) {
        while (!check(TokenType::RBRACKET) && !isAtEnd()) {
            if (check(TokenType::STRING)) {
                Token strToken = advance();
                stringElements.push_back(std::make_shared<StringLiteral>(std::get<std::string>(strToken.value)));
                
                if (match({TokenType::COMMA})) {
                    continue;
                } else if (check(TokenType::RBRACKET)) {
                    break;
                } else {
                    // Not a pure string array
                    allStrings = false;
                    break;
                }
            } else {
                // Not a string literal
                allStrings = false;
                break;
            }
        }
    }
    
    if (allStrings && !stringElements.empty()) {
        // This is a string array
        consume(TokenType::RBRACKET, "Expected ']' after string array literal");
        return std::make_shared<StringArrayLiteral>(stringElements);
    }
    
    // Restore position and parse as regular matrix
    current_ = savePos;
    
    auto matrix = std::make_shared<MatrixLiteral>();

    if (check(TokenType::RBRACKET)) {
        advance();
        return matrix;
    }

    std::vector<ExprPtr> currentRow;

    while (!check(TokenType::RBRACKET) && !isAtEnd()) {
        currentRow.push_back(parseExpression());

        if (match({TokenType::COMMA})) {
            continue;
        } else if (match({TokenType::SEMICOLON})) {
            matrix->rows.push_back(currentRow);
            currentRow.clear();
        } else if (check(TokenType::RBRACKET)) {
            break;
        } else if (isAtEnd()) {
            break;
        } else {
            // MATLAB allows whitespace as element separator
            // If next token can start an expression, treat it as a new element
            if (canStartExpression(peek().type)) {
                continue;
            }
            throw error(peek(), "Expected ',' or ';' or ']' in matrix literal");
        }
    }

    if (!currentRow.empty()) {
        matrix->rows.push_back(currentRow);
    }

    consume(TokenType::RBRACKET, "Expected ']' after matrix literal");
    return matrix;
}

ExprPtr Parser::parseCellLiteral() {
    std::vector<ExprPtr> elements;

    if (check(TokenType::RBRACE)) {
        advance();
        return std::make_shared<CellLiteral>(elements);
    }

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        elements.push_back(parseExpression());

        if (match({TokenType::COMMA})) {
            continue;
        } else if (check(TokenType::RBRACE)) {
            break;
        } else {
            throw error(peek(), "Expected ',' or '}' in cell literal");
        }
    }

    consume(TokenType::RBRACE, "Expected '}' after cell literal");
    return std::make_shared<CellLiteral>(elements);
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

Token Parser::peek() const {
    return tokens_[current_];
}

Token Parser::previous() const {
    return tokens_[current_ - 1];
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

bool Parser::canStartExpression(TokenType type) const {
    switch (type) {
        case TokenType::NUMBER:
        case TokenType::COMPLEX:
        case TokenType::STRING:
        case TokenType::IDENTIFIER:
        case TokenType::TRUE:
        case TokenType::FALSE:
        case TokenType::LPAREN:
        case TokenType::LBRACKET:
        case TokenType::LBRACE:
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::NOT:
            return true;
        default:
            return false;
    }
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON || 
            previous().type == TokenType::NEWLINE) return;
        
        switch (peek().type) {
            case TokenType::FUNCTION:
            case TokenType::FOR:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
                return;
            default:
                break;
        }
        
        advance();
    }
}

ParseError Parser::error(const Token& token, const std::string& message) {
    std::string errMsg = "[line " + std::to_string(token.line) + 
                        ", col " + std::to_string(token.column) + "] Error";
    if (token.type == TokenType::EOF_TOKEN) {
        errMsg += " at end";
    } else {
        errMsg += " at '" + token.lexeme + "'";
    }
    errMsg += ": " + message;
    return ParseError(errMsg);
}

}
