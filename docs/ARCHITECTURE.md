# CNLab 架构设计

## 概述

CNLab 采用**双模式架构**，既可以作为**独立语言解释器**运行，也可以作为**嵌入式脚本引擎**集成到其他软件中。

```
┌─────────────────────────────────────────────────────────────┐
│                        CNLab                                │
├─────────────────────────────────────────────────────────────┤
│  Mode 1: 独立解释器              Mode 2: 嵌入式引擎          │
│  ┌─────────────────────┐        ┌─────────────────────┐    │
│  │   cnlab.exe         │        │   Interpreter       │    │
│  │   (命令行工具)       │        │   (C++ API)         │    │
│  └──────────┬──────────┘        └──────────┬──────────┘    │
│             │                               │               │
│             └───────────────┬───────────────┘               │
│                             │                               │
│                    ┌────────▼────────┐                      │
│                    │  Interpreter    │                      │
│                    │  (核心解释器)    │                      │
│                    └────────┬────────┘                      │
│                             │                               │
│        ┌────────────────────┼────────────────────┐         │
│        │                    │                    │         │
│   ┌────▼────┐         ┌────▼────┐         ┌────▼────┐    │
│   │  Lexer  │         │ Parser  │         │Evaluator│    │
│   │(词法分析)│         │(语法分析)│         │(执行引擎)│    │
│   └─────────┘         └─────────┘         └─────────┘    │
│                                                             │
│   ┌─────────────────────────────────────────────────────┐  │
│   │              Matrix Engine (矩阵引擎)                │  │
│   │  - 基础矩阵运算  - 线性代数  - GPU加速  - 内存优化   │  │
│   └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 两种使用模式

### 模式一：独立解释器（命令行工具）

作为一门独立的脚本语言使用，类似 Python、MATLAB。

**使用场景：**
- 科学计算脚本
- 数据分析
- 算法原型验证
- 教学演示

**使用方式：**
```bash
# 执行脚本文件
cnlab script.m

# 执行代码字符串
cnlab -c "x = 5; y = x * 2"

# 进入交互式 REPL
cnlab

# 查看版本
cnlab -v
```

**入口点：** `src/main.cpp`

### 模式二：嵌入式引擎（C++ API）

作为底层解释器集成到其他 C++ 应用程序中。

**使用场景：**
- CAD/CAE 软件中的参数化建模
- 游戏引擎的脚本系统
- 自动化测试框架
- 可配置的业务逻辑
- 插件系统

**使用方式：**
```cpp
#include "Interpreter.hpp"

// 创建解释器
cnlab::Interpreter interpreter;

// 执行代码
interpreter.run("A = [1, 2; 3, 4]; B = inv(A)");

// 执行脚本文件
interpreter.runFile("script.m");

// 交互式 REPL
interpreter.runPrompt();
```

**入口点：** `include/Interpreter.hpp` → `Interpreter` 类

## 核心组件

### 1. Interpreter（解释器层）

**文件：** `include/Interpreter.hpp`, `src/Interpreter.cpp`

**职责：**
- 脚本加载和执行
- 交互式 REPL
- 语句级执行控制

**主要接口：**
```cpp
class Interpreter {
public:
    void run(const std::string& code);
    void runFile(const std::string& path);
    void runPrompt();
};
```

### 2. Lexer（词法分析器）

**文件：** `include/Lexer.hpp`, `src/Lexer.cpp`

**职责：**
- 源代码分词
- Token 生成
- 词法错误检测

### 3. Parser（语法分析器）

**文件：** `include/Parser.hpp`, `src/Parser.cpp`

**职责：**
- 语法分析
- AST（抽象语法树）构建
- 语法错误检测

### 4. Evaluator（执行引擎）

**文件：** `include/Evaluator.hpp`, `src/Evaluator.cpp`

**职责：**
- AST 遍历执行
- 变量管理
- 函数调用
- 内置函数实现

### 5. Matrix Engine（矩阵引擎）

**文件：** `include/Matrix.hpp`, `src/Matrix.cpp`

**职责：**
- 矩阵存储和内存管理
- 基础矩阵运算（+ - * /）
- 高级线性代数运算
- GPU 加速支持

## 嵌入式 API 详解

### 基础 API

```cpp
#include "Interpreter.hpp"
#include "Matrix.hpp"

using namespace cnlab;

// 创建解释器
Interpreter interpreter;

// 执行代码
interpreter.run("x = 5; y = x * 2");

// 执行脚本文件
interpreter.runFile("script.m");

// 交互式 REPL
interpreter.runPrompt();

// 矩阵操作
Matrix m1 = Matrix::zeros(3, 3);
Matrix m2 = Matrix::ones(3, 3);
Matrix m3 = m1 + m2;
```

### 矩阵 API

```cpp
// 创建矩阵
Matrix m(rows, cols);
Matrix z = Matrix::zeros(rows, cols);
Matrix o = Matrix::ones(rows, cols);
Matrix i = Matrix::eye(n);
Matrix r = Matrix::rand(rows, cols);

// 访问元素
m(i, j) = value;
double val = m(i, j);

// 矩阵运算
Matrix c = a + b;
Matrix c = a - b;
Matrix c = a * b;
Matrix c = a.transpose();
Matrix c = a.inverse();
double d = a.det();
```

## 集成示例

### 示例 1：简单嵌入式使用

```cpp
#include "Interpreter.hpp"
#include <iostream>

int main() {
    cnlab::Interpreter interpreter;
    
    // 执行代码
    interpreter.run("A = [1, 2; 3, 4]");
    interpreter.run("B = inv(A)");
    interpreter.run("disp('Matrix inversion done')");
    
    return 0;
}
```

### 示例 2：Qt 应用程序集成

```cpp
// 在 Qt 应用中嵌入脚本功能
class ScriptWidget : public QWidget {
    Q_OBJECT
    
    cnlab::Interpreter interpreter;
    
public:
    ScriptWidget() {
        // 设置输出重定向
        // ...
    }
    
    void runScript() {
        QString code = inputTextEdit->toPlainText();
        try {
            interpreter.run(code.toStdString());
        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error", e.what());
        }
    }
};
```

### 示例 3：游戏引擎脚本

```cpp
// 游戏对象行为脚本
class GameObject {
    cnlab::Interpreter scriptEngine;
    
public:
    void loadScript(const std::string& scriptPath) {
        scriptEngine.runFile(scriptPath);
    }
    
    void update(float deltaTime) {
        // 调用脚本更新函数
        scriptEngine.run("update(" + std::to_string(deltaTime) + ")");
    }
};
```

## 构建配置

### 独立解释器
```cmake
add_executable(cnlab src/main.cpp)
target_link_libraries(cnlab cnlab_interpreter cnlab_core)
```

### 嵌入式库
```cmake
# 静态库
add_library(cnlab_static STATIC 
    src/Interpreter.cpp
    src/Evaluator.cpp
    src/Parser.cpp
    src/Lexer.cpp
    src/AST.cpp
    src/Token.cpp
    src/Matrix.cpp
    # ... 其他源文件
)
```

## 扩展机制

### 注册 C++ 函数（未来版本）
```cpp
// 注册自定义函数到脚本环境
interpreter.registerFunction("myFunction", 
    [](double x, double y) -> double {
        return x * x + y * y;
    });

// 脚本中使用
// result = myFunction(3, 4)  // 返回 25
```

## 总结

CNLab 的双模式架构提供了极大的灵活性：

1. **独立模式**：作为完整的脚本语言使用，适合科学计算和快速原型
2. **嵌入式模式**：作为软件组件集成，适合需要脚本能力的应用程序

两种模式共享相同的核心解释器和矩阵引擎，确保行为一致性和代码复用。
