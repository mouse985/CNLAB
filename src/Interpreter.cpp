#include "Interpreter.hpp"
#include "Evaluator.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <variant>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

namespace cnlab {

Interpreter::Interpreter() {}

void Interpreter::run(const std::string& source) {
    try {
        execute(source);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void Interpreter::runFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << path << std::endl;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    run(buffer.str());
}

#ifdef _WIN32
void Interpreter::runFileWide(const std::wstring& path) {
    // Use wfstream for wide character path
    std::wifstream wfile(path);
    if (!wfile.is_open()) {
        // Convert path back to UTF-8 for error message
        int len = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string utf8Path(len, 0);
        WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, &utf8Path[0], len, nullptr, nullptr);
        utf8Path.resize(len - 1);
        std::cerr << "Could not open file: " << utf8Path << std::endl;
        return;
    }
    
    // Read as wide string then convert to UTF-8
    std::wstringstream wbuffer;
    wbuffer << wfile.rdbuf();
    wfile.close();
    
    std::wstring wsource = wbuffer.str();
    
    // Convert wide string to UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, wsource.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string source(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wsource.c_str(), -1, &source[0], len, nullptr, nullptr);
    source.resize(len - 1);
    
    run(source);
}
#endif

void Interpreter::runPrompt() {
    std::cout << "CNLab Interpreter v1.0.0" << std::endl;
    std::cout << "Type 'exit' to quit." << std::endl;
    std::cout << std::endl;
    
    std::string line;
    while (true) {
        std::cout << ">>> ";
        std::getline(std::cin, line);
        
        if (line == "exit" || line == "quit") {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        run(line);
    }
}

void Interpreter::execute(const std::string& source) {
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.scanTokens();
    
    Parser parser(tokens);
    auto program = parser.parse();
    
    for (auto& stmt : program->statements) {
        evaluator_.execute(stmt);
        
        if (auto* exprStmt = dynamic_cast<ExpressionStatement*>(stmt.get())) {
            if (auto* binaryExpr = dynamic_cast<BinaryExpression*>(exprStmt->expression.get())) {
                if (binaryExpr->op == "=") {
                    continue;
                }
            }
            
            Value result = evaluator_.getResult();
            if (!std::holds_alternative<std::monostate>(result)) {
                std::cout << Evaluator::valueToString(result) << std::endl;
            }
        }
    }
}

}
