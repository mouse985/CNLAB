#pragma once

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Evaluator.hpp"
#include <string>

#ifdef _WIN32
#include <string>
#endif

namespace cnlab {

class Interpreter {
public:
    Interpreter();
    
    void run(const std::string& source);
    void runFile(const std::string& path);
#ifdef _WIN32
    void runFileWide(const std::wstring& path);
#endif
    void runPrompt();
    
private:
    Evaluator evaluator_;
    
    void execute(const std::string& source);
};

}
