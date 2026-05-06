# CNLab语法差异说明

## MATLAB vs CNLab 语法差异

### 1. 注释符号

**MATLAB**:
```matlab
% 这是MATLAB注释
```

**CNLab**:
```matlab
# 这是CNLab注释
```

### 2. 语句分隔符

**MATLAB** (支持逗号作为语句分隔符):
```matlab
plot(x, y), xlabel('x'), ylabel('y'), title('title')
```

**CNLab** (不支持逗号分隔语句):
```matlab
# 方式1: 使用换行
plot(x, y)
xlabel('x')
ylabel('y')
title('title')

# 方式2: 使用分号
plot(x, y); xlabel('x'); ylabel('y'); title('title');
```

### 3. 矩阵元素分隔

**MATLAB** (支持空格或逗号):
```matlab
A = [1 2 3; 4 5 6]      # 空格分隔
B = [1, 2, 3; 4, 5, 6]  # 逗号分隔
axis([0 5 -1 1])        # 空格分隔
```

**CNLab** (仅支持逗号):
```matlab
A = [1, 2, 3; 4, 5, 6]  # 必须使用逗号
axis([0, 5, -1, 1])     # 必须使用逗号
```

### 4. 字符串引号

**MATLAB** (仅单引号):
```matlab
str = 'hello'
```

**CNLab** (单引号或双引号):
```matlab
str = 'hello'  # 单引号
str = "hello"  # 双引号
```

### 5. 范围表达式

**MATLAB**:
```matlab
x = [0:0.1:10]  # 方括号可选
```

**CNLab**:
```matlab
x = 0:0.1:10    # 推荐不加方括号
x = [0:0.1:10]  # 也支持
```

### 6. 元素级运算

**MATLAB** 和 **CNLab** 都支持:
```matlab
A .* B    # 元素级乘法
A ./ B    # 元素级除法
A .^ B    # 元素级幂运算
```

## 代码转换示例

### MATLAB代码:
```matlab
x = [0:0.01:5];
y = exp(-1.5*x).*sin(10*x);
subplot(1,2,1)
plot(x,y), xlabel('x'), ylabel('exp(–1.5x)*sin(10x)'), axis([0 5 -1 1])
```

### CNLab代码:
```matlab
x = 0:0.01:5
y = exp(-1.5*x).*sin(10*x)
subplot(1, 2, 1)
plot(x, y)
xlabel('x')
ylabel('exp(-1.5x)*sin(10x)')
axis([0, 5, -1, 1])
```

## 主要修改点

1. ✅ 注释符号: `%` → `#`
2. ✅ 语句分隔: 逗号改为换行或分号
3. ✅ 矩阵元素: 空格改为逗号
4. ✅ 范围表达式: 移除可选的方括号
5. ✅ 字符串: 单引号或双引号均可

## 兼容性统计

| 特性 | MATLAB | CNLab | 兼容性 |
|------|--------|-------|--------|
| 变量赋值 | `x = 5` | `x = 5` | ✅ 100% |
| 矩阵创建 | `[1, 2; 3, 4]` | `[1, 2; 3, 4]` | ✅ 100% |
| 元素级运算 | `.*`, `./`, `.^` | `.*`, `./`, `.^` | ✅ 100% |
| 控制流 | if/for/while | if/for/while | ✅ 100% |
| 函数定义 | function...end | function...end | ✅ 100% |
| 注释符号 | `%` | `#` | ⚠️ 差异 |
| 语句分隔 | `,` 或 `;` | 仅 `;` | ⚠️ 差异 |
| 矩阵元素 | 空格或`,` | 仅`,` | ⚠️ 差异 |

**总体兼容性**: 约90-95%（语法差异较小，易于转换）
