/*
 * CNLab 嵌入式使用示例
 * 演示如何将 CNLab 作为底层解释器集成到 C++ 应用程序中
 */

#include "CNLabEngine.hpp"
#include "Matrix.hpp"
#include <iostream>

using namespace cnlab;

// 示例1：简单执行代码
void example1_simple_execution() {
    std::cout << "=== Example 1: Simple Execution ===" << std::endl;
    
    CNLabEngine engine;
    
    // 执行代码
    auto result = engine.execute("x = 5; y = x * 2");
    
    if (result.success) {
        std::cout << "Execution successful!" << std::endl;
    } else {
        std::cout << "Error: " << result.error << std::endl;
    }
}

// 示例2：执行脚本文件
void example2_script_file() {
    std::cout << "\n=== Example 2: Script File ===" << std::endl;
    
    CNLabEngine engine;
    
    // 执行脚本文件
    auto result = engine.executeFile("test_script.m");
    
    if (result.success) {
        std::cout << "Script executed successfully!" << std::endl;
    } else {
        std::cout << "Error: " << result.error << std::endl;
    }
}

// 示例3：使用回调函数
void example3_callbacks() {
    std::cout << "\n=== Example 3: Callbacks ===" << std::endl;
    
    EngineConfig config;
    config.onOutput = [](const std::string& str) {
        std::cout << "[OUTPUT] " << str;
    };
    config.onError = [](const std::string& str) {
        std::cout << "[ERROR] " << str;
    };
    
    CNLabEngine engine(config);
    engine.execute("A = [1, 2; 3, 4]");
}

// 示例4：错误处理
void example4_error_handling() {
    std::cout << "\n=== Example 4: Error Handling ===" << std::endl;
    
    CNLabEngine engine;
    
    // 正常代码
    auto result1 = engine.execute("A = [1, 2; 3, 4]");
    std::cout << "Success: " << result1.success << std::endl;
    
    // 错误代码（维度不匹配）
    auto result2 = engine.execute("B = [1, 2, 3]; C = A + B");
    std::cout << "Success: " << result2.success << std::endl;
    if (!result2.success) {
        std::cout << "Error message: " << result2.error << std::endl;
    }
}

// 示例5：版本信息
void example5_version() {
    std::cout << "\n=== Example 5: Version Info ===" << std::endl;
    
    std::cout << "Version: " << CNLabEngine::getVersion() << std::endl;
    std::cout << "Build Info: " << CNLabEngine::getBuildInfo() << std::endl;
}

int main() {
    std::cout << "CNLab Embedded Examples" << std::endl;
    std::cout << "=======================" << std::endl;
    
    example1_simple_execution();
    example2_script_file();
    example3_callbacks();
    example4_error_handling();
    example5_version();
    
    std::cout << "\nAll examples completed!" << std::endl;
    
    return 0;
}
