# CNLab 语法扩展指南

本文档说明如何扩展MatlabCN的语法功能�?
## 架构概览

```
源代�?�?Lexer(词法分析) �?Tokens �?Parser(语法分析) �?AST �?Evaluator(执行) �?结果
```

要扩展语法，通常需要修改：
1. **Token.hpp/cpp** - 添加新token类型
2. **Lexer.cpp** - 识别新token
3. **AST.hpp/cpp** - 添加新AST节点
4. **Parser.cpp** - 解析新语�?5. **Evaluator.cpp** - 执行新功�?
---

## 示例1：添加新运算�?`.^`（点幂运算）

### 步骤1：添加Token类型

**文件：include/Token.hpp**

```cpp
enum class TokenType {
    // ... 现有token ...
    DOT_CARET,    // 添加 .^
    // ...
};
```

### 步骤2：Lexer识别新token

**文件：src/Lexer.cpp**

�?`scanToken()` 函数中添加：

```cpp
case '.':
    if (match('^')) {
        addToken(TokenType::DOT_CARET);
    } else if (isDigit(peek())) {
        // 现有的小数处�?    } else {
        addToken(TokenType::DOT);
    }
    break;
```

### 步骤3：Parser添加优先�?
**文件：src/Parser.cpp**

�?`parsePower()` 或创建新�?`parseDotPower()`�?
```cpp
ExprPtr Parser::parseMultiplicative() {
    ExprPtr expr = parseDotPower();  // 改为调用新的函数
    
    while (match({TokenType::STAR, TokenType::SLASH})) {
        // ...
    }
    return expr;
}

// 新增函数
ExprPtr Parser::parseDotPower() {
    ExprPtr expr = parsePower();
    
    while (match({TokenType::DOT_CARET})) {
        std::string op = ".^";
        ExprPtr right = parsePower();
        expr = std::make_shared<BinaryExpression>(expr, op, right);
    }
    return expr;
}
```

### 步骤4：Evaluator实现运算

**文件：src/Evaluator.cpp**

�?`visit(BinaryExpression& node)` 中添加：

```cpp
} else if (node.op == ".^") {
    result_ = dotPower(left, right);
}
```

添加新函数：

```cpp
Value Evaluator::dotPower(const Value& left, const Value& right) {
    Matrix a = toMatrix(left);
    double exp = toDouble(right);
    
    Matrix result(a.rows(), a.cols());
    for (size_t i = 0; i < a.rows(); ++i) {
        for (size_t j = 0; j < a.cols(); ++j) {
            result(i, j) = std::pow(a(i, j), exp);
        }
    }
    return result;
}
```

---

## 示例2：添加新语句类型（break�?
### 步骤1：添加Token

**include/Token.hpp**
```cpp
enum class TokenType {
    // ...
    BREAK,    // 添加
    // ...
};
```

**src/Lexer.cpp** 在keywords中添加：
```cpp
{"break", TokenType::BREAK},
```

### 步骤2：添加AST节点

**include/AST.hpp**

```cpp
struct BreakStatement : Statement {
    void accept(ASTVisitor& visitor) override;
};
```

**src/AST.cpp**
```cpp
void BreakStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
```

**include/AST.hpp** 在ASTVisitor中添加：
```cpp
virtual void visit(BreakStatement& node) = 0;
```

### 步骤3：Parser解析

**src/Parser.cpp** �?`parseStatement()` 中：
```cpp
if (match({TokenType::BREAK})) return parseBreakStatement();
```

添加函数�?```cpp
std::shared_ptr<Statement> Parser::parseBreakStatement() {
    return std::make_shared<BreakStatement>();
}
```

### 步骤4：Evaluator执行

**include/Evaluator.hpp** 添加�?```cpp
void visit(BreakStatement& node) override;
bool break_;
```

**src/Evaluator.cpp**�?```cpp
void Evaluator::visit(BreakStatement& node) {
    break_ = true;
}
```

修改循环执行�?```cpp
void Evaluator::visit(WhileStatement& node) {
    while (toBool(evaluate(node.condition))) {
        for (auto& stmt : node.body) {
            execute(stmt);
            if (break_) {
                break_ = false;
                return;
            }
            if (returned_) return;
        }
    }
}
```

---

## 示例3：添加新内置函数

以添�?`reshape` 函数为例�?
### 步骤：Evaluator中添�?
**src/Evaluator.cpp** �?`callFunction()` 中：

```cpp
if (name == "reshape") {
    if (args.size() == 3) {
        Matrix m = toMatrix(args[0]);
        size_t rows = static_cast<size_t>(toDouble(args[1]));
        size_t cols = static_cast<size_t>(toDouble(args[2]));
        return m.reshape(rows, cols);
    }
    throw RuntimeError("reshape requires 3 arguments");
}
```

使用�?```matlab
A = 1:12
B = reshape(A, 3, 4)
```

---

## 示例4：添加复合赋值运算符

### 添加 `+=` 运算�?
**步骤1：Token.hpp**
```cpp
PLUS_ASSIGN,  // +=
```

**步骤2：Lexer.cpp**
```cpp
case '+':
    addToken(match('=') ? TokenType::PLUS_ASSIGN : TokenType::PLUS);
    break;
```

**步骤3：Parser.cpp**

修改 `parseAssignmentOrExpression()`�?
```cpp
if (auto* id = dynamic_cast<Identifier*>(expr.get())) {
    if (match({TokenType::ASSIGN})) {
        // ...
    } else if (match({TokenType::PLUS_ASSIGN})) {
        // x += y 转换�?x = x + y
        ExprPtr value = parseExpression();
        ExprPtr addExpr = std::make_shared<BinaryExpression>(
            std::make_shared<Identifier>(id->name), "+", value);
        return std::make_shared<AssignmentStatement>(id->name, addExpr);
    }
}
```

---

## 扩展要点总结

### 1. 修改顺序

```
Token定义 �?Lexer识别 �?AST节点 �?Parser解析 �?Evaluator执行
```

### 2. 文件对应关系

| 功能 | 需要修改的文件 |
|------|---------------|
| 新运算符 | Token.hpp, Lexer.cpp, Parser.cpp, Evaluator.cpp |
| 新关键字 | Token.hpp, Lexer.cpp, Parser.cpp, Evaluator.cpp |
| 新语�?| Token.hpp, AST.hpp/cpp, Parser.cpp, Evaluator.cpp |
| 新函�?| Evaluator.cpp |
| 新数据类�?| Matrix.hpp/cpp, Evaluator.hpp/cpp |

### 3. 调试技�?
**打印Token流：**
```cpp
// 在Interpreter.cpp中临时添�?for (const auto& token : tokens) {
    std::cout << token.toString() << std::endl;
}
```

**打印AST�?*
```cpp
// 添加Visitor打印AST结构
class ASTPrinter : public ASTVisitor {
    void visit(BinaryExpression& node) override {
        std::cout << "Binary(" << node.op << ")" << std::endl;
        node.left->accept(*this);
        node.right->accept(*this);
    }
    // ... 其他visit函数
};
```

### 4. 常见错误

| 错误 | 原因 | 解决 |
|------|------|------|
| Token未识�?| Lexer未添加case | 检查Lexer.cpp的switch |
| 解析失败 | 优先级错�?| 检查Parser的调用链 |
| 执行错误 | AST节点未处�?| 检查Evaluator的visit函数 |
| 链接错误 | AST节点未实�?| 检查AST.cpp是否有对应函�?|

---

## 高级扩展

### 添加�?对象系统

需要：
1. 新的Token：CLASS, METHOD, PROPERTY�?2. 新的AST节点：ClassDeclaration, MethodCall�?3. 环境扩展：支持对象作用域
4. 内置类系�?
### 添加模块/包系�?
需要：
1. import语句
2. 命名空间管理
3. 模块加载�?
### 添加JIT编译

需要：
1. LLVM集成
2. AST到IR的转�?3. 运行时编译执�?
---

## 扩展示例5：添加GPU运算支持

### 步骤1：创建GPU接口头文�?
**include/GPUSimple.hpp**
```cpp
#pragma once
#include "Matrix.hpp"

namespace cnlab {
namespace gpu {

class GPUContext {
public:
    static GPUContext& getInstance();
    bool initialize();
    void shutdown();
    bool isAvailable() const;
};

Matrix multiply(const Matrix& A, const Matrix& B);
void enableGPU();
std::string getGPUInfo();

}
}
```

### 步骤2：实现GPU封装�?
**src/GPUSimple.cpp**
```cpp
#include "GPUSimple.hpp"

namespace cnlab {
namespace gpu {

GPUContext& GPUContext::getInstance() {
    static GPUContext instance;
    return instance;
}

bool GPUContext::initialize() {
    // OpenCL/CUDA初始�?    return false; // 当前为占位实�?}

Matrix multiply(const Matrix& A, const Matrix& B) {
    // GPU矩阵乘法实现
    return A * B; // 回退到CPU
}

}
}
```

### 步骤3：添加脚本函�?
**src/Evaluator.cpp**
```cpp
if (name == "enable_gpu") {
    matlab::gpu::enableGPU();
    return 1.0;
}

if (name == "gpu_info") {
    std::cout << matlab::gpu::getGPUInfo() << std::endl;
    return 0.0;
}
```

### 步骤4：更新CMake

**CMakeLists.txt**
```cmake
set(INTERPRETER_SOURCES
    # ... 现有源文�?...
    src/GPUSimple.cpp
)

# 可选：查找OpenCL
find_package(OpenCL)
if(OpenCL_FOUND)
    target_link_libraries(matlab_core OpenCL::OpenCL)
endif()
```

---

## 扩展示例代码�?
位置：`examples/extensions/`

每个扩展一个目录，包含�?- 修改说明.md
- 补丁文件.diff
- 测试用例.m
