# MATLAB官方示例兼容性增强计划

## 问题概述

MATLAB官方示例代码在CNLab中无法运行，需要增强解析器的兼容性。

## 需要解决的语法问题

### 1. 方括号内的范围表达式 (高优先级)

**MATLAB代码**:
```matlab
x = [0:0.01:5];  # 方括号包裹的范围表达式
```

**当前行为**: 解析失败
**期望行为**: 等同于 `x = 0:0.01:5`

**实现方案**:
- 修改 `parseMatrixLiteral()` 函数
- 在解析矩阵元素时，支持范围表达式
- 如果整个方括号内只有一个范围表达式，直接返回该范围表达式

### 2. 逗号作为语句分隔符 (高优先级)

**MATLAB代码**:
```matlab
plot(x,y), xlabel('x'), ylabel('y')
```

**当前行为**: 解析失败，不支持逗号分隔语句
**期望行为**: 逗号等同于换行，分隔多个语句

**实现方案**:
- 修改 `parseStatement()` 或 `parse()` 函数
- 在语句解析后，检查是否有逗号
- 如果有逗号，继续解析下一个语句
- 将所有语句组合成一个语句块

### 3. 矩阵元素空格分隔 (中优先级)

**MATLAB代码**:
```matlab
axis([0 5 -1 1])  # 空格分隔矩阵元素
```

**当前行为**: 解析失败，要求逗号分隔
**期望行为**: 空格等同于逗号，作为矩阵元素分隔符

**实现方案**:
- 修改 `parseMatrixLiteral()` 函数
- 在解析矩阵元素时，如果两个表达式之间没有逗号或分号
- 但下一个token可以开始一个表达式，则视为新元素的开始
- 需要实现 `canStartExpression()` 函数来检测

## 实现步骤

### 阶段1: 方括号内的范围表达式

**文件**: `src/Parser.cpp`

**修改内容**:
1. 在 `parseMatrixLiteral()` 中，第一个元素解析前检查是否是范围表达式
2. 如果是范围表达式且后面直接是 `]`，则返回该范围表达式
3. 不需要创建矩阵字面量

**代码逻辑**:
```cpp
ExprPtr Parser::parseMatrixLiteral() {
    // 保存位置以便回溯
    size_t savePos = current_;
    
    // 尝试解析为范围表达式
    if (!check(TokenType::RBRACKET)) {
        auto expr = parseExpression();
        
        // 检查是否是范围表达式
        if (auto* rangeExpr = dynamic_cast<RangeExpression*>(expr.get())) {
            if (check(TokenType::RBRACKET)) {
                // [start:step:end] 形式，直接返回范围表达式
                advance(); // consume ']'
                return expr;
            }
        }
        
        // 不是范围表达式或后面还有其他元素，回溯
        current_ = savePos;
    }
    
    // 继续原有的矩阵解析逻辑...
}
```

### 阶段2: 逗号作为语句分隔符

**文件**: `src/Parser.cpp`

**修改内容**:
1. 修改 `parse()` 函数的主循环
2. 在解析完一个语句后，检查是否有逗号
3. 如果有逗号，消费它并继续解析下一个语句

**代码逻辑**:
```cpp
std::shared_ptr<Program> Parser::parse() {
    auto program = std::make_shared<Program>();
    
    while (!isAtEnd()) {
        try {
            auto stmt = parseStatement();
            if (stmt) {
                program->statements.push_back(stmt);
            }
            
            // 支持逗号作为语句分隔符
            while (match({TokenType::COMMA})) {
                // 跳过逗号，继续解析下一个语句
                if (!isAtEnd() && !check(TokenType::NEWLINE) && !check(TokenType::SEMICOLON)) {
                    auto nextStmt = parseStatement();
                    if (nextStmt) {
                        program->statements.push_back(nextStmt);
                    }
                }
            }
        } catch (const ParseError& e) {
            std::cerr << "Parse error: " << e.what() << std::endl;
            synchronize();
        }
    }
    
    return program;
}
```

### 阶段3: 矩阵元素空格分隔

**文件**: `src/Parser.cpp`

**修改内容**:
1. 修改 `parseMatrixLiteral()` 中的元素解析逻辑
2. 在遇到非逗号/分号分隔符时，检查是否可以开始新表达式

**代码逻辑**:
```cpp
// 在 parseMatrixLiteral() 的 while 循环中
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
        // MATLAB允许空格作为元素分隔符
        // 如果下一个token可以开始表达式，视为新元素
        if (canStartExpression(peek().type)) {
            continue;  // 继续循环，解析下一个元素
        }
        throw error(peek(), "Expected ',' or ';' or ']' in matrix literal");
    }
}
```

**需要完善 `canStartExpression()`**:
```cpp
bool Parser::canStartExpression(TokenType type) {
    switch (type) {
        case TokenType::NUMBER:
        case TokenType::IDENTIFIER:
        case TokenType::STRING:
        case TokenType::LPAREN:
        case TokenType::LBRACKET:
        case TokenType::LBRACE:
        case TokenType::MINUS:
        case TokenType::PLUS:
        case TokenType::NOT:
        case TokenType::TILDE:
            return true;
        default:
            return false;
    }
}
```

## 测试计划

### 测试用例1: 方括号内的范围表达式
```matlab
x = [0:0.01:5]
y = [1:10]
z = [0:2:100]
```

### 测试用例2: 逗号作为语句分隔符
```matlab
a = 1, b = 2, c = 3
plot(x, y), xlabel('x'), ylabel('y')
```

### 测试用例3: 矩阵元素空格分隔
```matlab
A = [1 2 3; 4 5 6]
axis([0 5 -1 1])
B = [1 2 3 4 5]
```

### 测试用例4: 完整MATLAB示例
```matlab
x = [0:0.01:5];
y = exp(-1.5*x).*sin(10*x);
subplot(1,2,1)
plot(x,y), xlabel('x'),ylabel('exp(-1.5x)*sin(10x)'),axis([0 5 -1 1])
y = exp(-2*x).*sin(10*x);
subplot(1,2,2)
plot(x,y),xlabel('x'),ylabel('exp(-2x)*sin(10x)'),axis([0 5 -1 1])
```

## 风险评估

1. **向后兼容性**: 这些修改应该保持向后兼容，只是增加了新的语法支持
2. **性能影响**: 额外的检查可能会略微影响解析速度，但影响应该很小
3. **歧义处理**: 需要仔细处理可能的语法歧义，例如 `[1:5]` 是范围还是矩阵

## 预期结果

完成这些修改后，MATLAB官方示例代码应该可以直接在CNLab中运行，无需修改。
