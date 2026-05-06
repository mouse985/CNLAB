# test2.m 错误分析

## 代码内容

```matlab
x = [-10 : 0.01: 10];
y = 3*x.^4 + 2 * x.^3 + 7 * x.^2 + 2 * x + 9;
g = 5 * x.^3 + 9 * x + 2;
plot(x, y, 'r', x, g, 'g')
show()
```

## 错误信息

```
Error: Cannot convert to scalar
```

## 分析过程

### 第1行: `x = [-10 : 0.01: 10];`

这行创建了一个范围表达式，应该可以正常工作。
- `[-10 : 0.01: 10]` 会被解析为范围表达式
- 方括号内的范围表达式应该返回一个矩阵

### 第2行: `y = 3*x.^4 + 2 * x.^3 + 7 * x.^2 + 2 * x + 9;`

这行包含多个元素级运算：
- `x.^4` - 元素级幂运算
- `3*x.^4` - 矩阵与标量乘法
- `+` 运算

**问题定位**:

在 `elementWisePow` 函数中：

```cpp
Value Evaluator::elementWisePow(const Value& left, const Value& right) {
    // ...
    } else if (leftIsMatrix) {
        Matrix m = std::get<Matrix>(left);
        double exp = toDouble(right);  // <-- 这里可能出错
        // ...
    }
    // ...
}
```

当执行 `x.^4` 时：
- `left` 是矩阵 `x`
- `right` 应该是标量 `4`
- 但 `toDouble(right)` 抛出了 "Cannot convert to scalar" 异常

这意味着 `right` 参数（应该是 `4`）没有被正确识别为标量。

### 可能的原因

1. **解析问题**: `4` 这个数字在解析时可能没有被正确识别为标量值
2. **类型问题**: `right` 可能是一个1x1矩阵而不是标量 `double`
3. **运算符优先级**: 表达式 `3*x.^4` 的解析可能有问题

### 测试验证

让我检查简单的幂运算是否工作：

```matlab
x = [1, 2, 3]
y = x.^2
disp(y)
```

如果这个简单的测试也失败，说明是 `.^` 运算符的基本实现有问题。

## 结论

错误发生在元素级幂运算 `.^` 的实现中。当右边是标量时，`toDouble()` 函数无法将其转换为 `double` 类型。

可能的原因：
1. 标量 `4` 在解析时被当作了 1x1 矩阵
2. `toDouble()` 函数没有正确处理 1x1 矩阵的情况

实际上查看 `toDouble()` 的实现：

```cpp
double Evaluator::toDouble(const Value& value) {
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    if (std::holds_alternative<Matrix>(value)) {
        const Matrix& m = std::get<Matrix>(value);
        if (m.isScalar()) {
            return m(0, 0);
        }
    }
    throw RuntimeError("Cannot convert to scalar");
}
```

这个实现看起来是正确的，它会检查 1x1 矩阵。所以问题可能是 `right` 既不是 `double` 也不是 `Matrix`，或者是空的 `Value`。

## 建议修复

需要在 `elementWisePow` 函数中添加更详细的错误处理，或者检查为什么 `right` 参数没有被正确传递。
