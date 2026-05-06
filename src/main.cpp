#include "Interpreter.hpp"
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>

// Convert ANSI string to UTF-8 (for command line arguments)
std::string ansiToUtf8(const std::string& ansi) {
    if (ansi.empty()) return "";
    
    // ANSI to UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return "";
    
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, &wstr[0], wlen);
    wstr.resize(wlen - 1);
    
    // UTF-16 to UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return "";
    
    std::string utf8(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    utf8.resize(len - 1);
    
    return utf8;
}

// Convert UTF-8 string to wide string (for file paths)
std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return L"";
    
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], wlen);
    wstr.resize(wlen - 1);
    
    return wstr;
}

#endif

void printVersion() {
    std::cout << "CNLab Interpreter v1.0.0" << std::endl;
    std::cout << "A MATLAB-like scientific computing environment" << std::endl;
}

void printHelp() {
    std::cout << "Usage: cnlab [options] [script]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -c <code>    Execute code string" << std::endl;
    std::cout << "  -v           Show version" << std::endl;
    std::cout << "  -h           Show this help" << std::endl;
    std::cout << "  -i           Force interactive mode" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  cnlab script.m              Run script file" << std::endl;
    std::cout << "  cnlab -c \"x = 5; disp(x)\"  Execute code" << std::endl;
    std::cout << "  cnlab                       Start REPL" << std::endl;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Set UTF-8 encoding for Windows console
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // Set C++ locale to UTF-8
    std::locale::global(std::locale(".UTF-8"));
    std::cout.imbue(std::locale(".UTF-8"));
    std::cerr.imbue(std::locale(".UTF-8"));
#endif
    
    cnlab::Interpreter interpreter;
    
    if (argc == 1) {
        // No arguments: interactive mode
        printVersion();
        std::cout << "Type 'exit' to quit." << std::endl;
        std::cout << std::endl;
        interpreter.runPrompt();
    } else if (argc == 2) {
#ifdef _WIN32
        std::string arg = ansiToUtf8(argv[1]);
#else
        std::string arg = argv[1];
#endif
        if (arg == "-v" || arg == "--version") {
            printVersion();
        } else if (arg == "-h" || arg == "--help") {
            printHelp();
        } else if (arg == "-i" || arg == "--interactive") {
            printVersion();
            std::cout << "Type 'exit' to quit." << std::endl;
            std::cout << std::endl;
            interpreter.runPrompt();
        } else {
            // Run script file
#ifdef _WIN32
            interpreter.runFileWide(utf8ToWide(arg));
#else
            interpreter.runFile(arg);
#endif
        }
    } else if (argc == 3) {
#ifdef _WIN32
        std::string arg = ansiToUtf8(argv[1]);
        std::string code = ansiToUtf8(argv[2]);
#else
        std::string arg = argv[1];
        std::string code = argv[2];
#endif
        if (arg == "-c" || arg == "--code") {
            // Execute code string
            interpreter.run(code);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printHelp();
            return 1;
        }
    } else {
        std::cerr << "Too many arguments" << std::endl;
        printHelp();
        return 1;
    }
    
    return 0;
}
