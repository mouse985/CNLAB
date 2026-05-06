#include "Lexer.hpp"
#include <cctype>
#include <stdexcept>

namespace cnlab {

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"elseif", TokenType::ELSEIF},
    {"for", TokenType::FOR},
    {"while", TokenType::WHILE},
    {"function", TokenType::FUNCTION},
    {"end", TokenType::END},
    {"return", TokenType::RETURN},
    {"true", TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"and", TokenType::AND},
    {"or", TokenType::OR},
    {"not", TokenType::NOT},
    {"break", TokenType::BREAK},
    {"continue", TokenType::CONTINUE},
    {"switch", TokenType::SWITCH},
    {"case", TokenType::CASE},
    {"otherwise", TokenType::OTHERWISE},
    {"try", TokenType::TRY},
    {"catch", TokenType::CATCH},
    {"global", TokenType::GLOBAL},
    {"varargin", TokenType::VARARGIN},
    {"varargout", TokenType::VARARGOUT},
    {"nargin", TokenType::NARGIN},
    {"nargout", TokenType::NARGOUT},
    {"struct", TokenType::STRUCT},
    {"cell", TokenType::CELL},
    {"table", TokenType::TABLE},
    {"categorical", TokenType::CATEGORICAL},
    {"datetime", TokenType::DATETIME},
    {"duration", TokenType::DURATION}
};

Lexer::Lexer(const std::string& source)
    : source_(source), start_(0), current_(0), line_(1), column_(1) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        start_ = current_;
        scanToken();
    }
    
    tokens_.emplace_back(TokenType::EOF_TOKEN, "", line_, column_);
    return tokens_;
}

void Lexer::scanToken() {
    skipWhitespace();
    
    if (isAtEnd()) return;
    
    start_ = current_;
    char c = advance();
    
    switch (c) {
        case '(': addToken(TokenType::LPAREN); break;
        case ')': addToken(TokenType::RPAREN); break;
        case '[': addToken(TokenType::LBRACKET); break;
        case ']': addToken(TokenType::RBRACKET); break;
        case '{': addToken(TokenType::LBRACE); break;
        case '}': addToken(TokenType::RBRACE); break;
        case ',': addToken(TokenType::COMMA); break;
        case ';': addToken(TokenType::SEMICOLON); break;
        case ':': addToken(TokenType::COLON); break;
        case '+': addToken(TokenType::PLUS); break;
        case '*': addToken(TokenType::STAR); break;
        case '/': addToken(TokenType::SLASH); break;
        case '\\': addToken(TokenType::BACKSLASH); break;
        case '%': addToken(TokenType::MODULO); break;
        case '^': addToken(TokenType::CARET); break;
        case '\'':
            // Check if this is a char array literal (e.g., 'hello')
            // or a transpose operator (e.g., A')
            // In MATLAB, ' at the start of an expression or after an operator is a char array
            // We need to look ahead to find the matching closing quote
            // If there's a matching ' before end of line/statement, it's a char array
            {
                size_t lookAhead = current_;
                bool isCharArray = false;
                while (lookAhead < source_.length()) {
                    char c = source_[lookAhead];
                    if (c == '\n' || c == ';' || c == ',') {
                        break; // End of statement without finding closing '
                    }
                    if (c == '\'') {
                        // Found closing quote - check if it's not '' (empty string)
                        if (lookAhead > current_ || source_[lookAhead - 1] != '\'') {
                            isCharArray = true;
                            break;
                        }
                    }
                    lookAhead++;
                }
                
                if (isCharArray) {
                    charArray();
                } else {
                    addToken(TokenType::TRANSPOSE);
                }
            }
            break;
        case '@': addToken(TokenType::AT); break;
        case '#': skipComment(); break;
        
        case '-':
            addToken(match('=') ? TokenType::MINUS : TokenType::MINUS);
            break;
        
        case '=':
            addToken(match('=') ? TokenType::EQUAL : TokenType::ASSIGN);
            break;
        
        case '<':
            addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
            break;
        
        case '>':
            addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
            break;
        
        case '~':
            if (match('=')) {
                addToken(TokenType::NOT_EQUAL);
            } else {
                addToken(TokenType::TILDE);
            }
            break;
        
        case '&':
            if (match('&')) {
                addToken(TokenType::AND);
            } else {
                addToken(TokenType::ELEMENT_AND);
            }
            break;
        
        case '|':
            if (match('|')) {
                addToken(TokenType::OR);
            } else {
                addToken(TokenType::ELEMENT_OR);
            }
            break;
        
        case '.':
            if (match('*')) {
                addToken(TokenType::ELEMENT_MUL);
            } else if (match('/')) {
                addToken(TokenType::ELEMENT_DIV);
            } else if (match('^')) {
                addToken(TokenType::ELEMENT_POW);
            } else if (match('\'')) {
                addToken(TokenType::TRANSPOSE_NON_CONJ);
            } else if (isDigit(peek())) {
                current_--;
                column_--;
                number();
            } else {
                addToken(TokenType::DOT);
            }
            break;
        
        case '\n':
            line_++;
            column_ = 1;
            addToken(TokenType::NEWLINE);
            break;
        
        case '"':
            if (peek() == '"' && peekNext() == '"') {
                advance();
                advance();
                skipMultiLineComment();
            } else {
                string();
            }
            break;
        
        default:
            if (isDigit(c)) {
                number();
            } else if (isAlpha(c)) {
                identifier();
            } else {
                addToken(TokenType::INVALID);
            }
            break;
    }
}

void Lexer::addToken(TokenType type) {
    std::string text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, line_, column_ - text.length());
}

void Lexer::addToken(TokenType type, double value) {
    std::string text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, value, line_, column_ - text.length());
}

void Lexer::addToken(TokenType type, const std::string& value) {
    std::string text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, value, line_, column_ - text.length());
}

void Lexer::addToken(TokenType type, bool value) {
    std::string text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, value, line_, column_ - text.length());
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.length();
}

char Lexer::advance() {
    column_++;
    return source_[current_++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

char Lexer::peekNextNext() const {
    if (current_ + 2 >= source_.length()) return '\0';
    return source_[current_ + 2];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source_[current_] != expected) return false;
    
    current_++;
    column_++;
    return true;
}

void Lexer::string() {
    char quote = source_[current_ - 1];
    std::string value;

    while (peek() != quote && !isAtEnd()) {
        if (peek() == '\n') {
            line_++;
            advance();
        } else if (peek() == '\\') {
            advance();
            char escaped = peek();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                case '0': value += '\0'; break;
                default: value += escaped; break;
            }
            advance();
        } else {
            value += peek();
            advance();
        }
    }

    if (isAtEnd()) {
        throw std::runtime_error("Unterminated string");
    }

    advance();
    addToken(TokenType::STRING, value);
}

void Lexer::charArray() {
    // Single-quoted char array (e.g., 'hello')
    std::string value;

    while (peek() != '\'' && !isAtEnd()) {
        if (peek() == '\n') {
            line_++;
            advance();
        } else if (peek() == '\\') {
            advance();
            char escaped = peek();
            switch (escaped) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '\'': value += '\''; break;
                case '"': value += '"'; break;
                case '0': value += '\0'; break;
                default: value += escaped; break;
            }
            advance();
        } else {
            value += peek();
            advance();
        }
    }

    if (isAtEnd()) {
        throw std::runtime_error("Unterminated character array");
    }

    advance(); // consume closing '
    addToken(TokenType::CHAR_ARRAY, value);
}

void Lexer::number() {
    while (isDigit(peek())) advance();
    
    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) advance();
    }
    
    if (peek() == 'e' || peek() == 'E') {
        advance();
        if (peek() == '+' || peek() == '-') advance();
        while (isDigit(peek())) advance();
    }
    
    // Check for complex number suffix 'i' or 'j'
    if (peek() == 'i' || peek() == 'j') {
        advance();
        std::string numStr = source_.substr(start_, current_ - start_);
        addToken(TokenType::COMPLEX, numStr);
        return;
    }
    
    std::string numStr = source_.substr(start_, current_ - start_);
    double value = std::stod(numStr);
    addToken(TokenType::NUMBER, value);
}

void Lexer::identifier() {
    while (isAlphaNumeric(peek())) advance();
    
    std::string text = source_.substr(start_, current_ - start_);
    
    auto it = keywords_.find(text);
    if (it != keywords_.end()) {
        TokenType type = it->second;
        if (type == TokenType::TRUE) {
            addToken(type, true);
        } else if (type == TokenType::FALSE) {
            addToken(type, false);
        } else {
            addToken(type);
        }
    } else {
        addToken(TokenType::IDENTIFIER, text);
    }
}

void Lexer::skipWhitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            default:
                return;
        }
    }
}

void Lexer::skipComment() {
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }
}

void Lexer::skipMultiLineComment() {
    while (!isAtEnd()) {
        if (peek() == '\n') {
            line_++;
        }
        if (peek() == '"' && peekNext() == '"' && peekNextNext() == '"') {
            advance();
            advance();
            advance();
            return;
        }
        advance();
    }
    // If we reach here, the multi-line comment is unterminated.
    // We could throw an error, but treating it as a comment till EOF is also common.
}

bool Lexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

}
