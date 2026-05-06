# CNLab 编程教程

欢迎来到 CNLab 编程教程！本教程将带你从零开始学习 CNLab 脚本语言，掌握科学计算的核心技能。

---

## 目录

1. [第一章：起步](#第一章起步)
2. [第二章：变量与数据类型](#第二章变量与数据类型)
3. [第三章：矩阵基础](#第三章矩阵基础)
4. [第四章：矩阵运算](#第四章矩阵运算)
5. [第五章：控制流](#第五章控制流)
6. [第六章：函数](#第六章函数)
7. [第七章：线性代数](#第七章线性代数)
8. [第八章：GPU加速](#第八章gpu加速)
9. [第九章：高级数据类型](#第九章高级数据类型)

---

## 与MATLAB的主要差异

| 特性 | MATLAB | CNLab | 说明 |
|------|--------|-------|------|
| 注释符号 | `%` | `#` | CNLab使用Python风格的注释 |
| 语句分隔符 | `,` 或 `;` | 仅 `;` 或换行 | CNLab不支持逗号分隔语句 |
| 矩阵元素分隔 | 空格或`,` | 仅`,` | CNLab矩阵元素必须用逗号 |
| 字符串引号 | 单引号`'` | 单引号或双引号 | CNLab更灵活 |

### 示例对比

**MATLAB代码**:
```matlab
% MATLAB注释
x = [0:0.1:10];
y = sin(x);
plot(x, y), xlabel('x'), ylabel('sin(x)'), axis([0 10 -1 1])
```

**CNLab代码**:
```matlab
# CNLab注释
x = 0:0.1:10
y = sin(x)
plot(x, y)
xlabel('x')
ylabel('sin(x)')
axis([0, 10, -1, 1])
```

---

## 第一章：起步

### 1.1 你的第一个程序

让我们从最简单的程序开始：

```matlab
# 这是我的第一个 CNLab 程序
disp("Hello, CNLab!")

"""
这是一个多行注释的例子
它可以包含多行文字
或者暂时屏蔽某段代码
"""
```

运行这段代码，你会看到输出：
```
Hello, CNLab!
```

**知识点：**
- `#` 开头的行是单行注释，程序会忽略它
- `"""` 包裹的内容是多行注释，可跨越多行
- `disp()` 是一个函数，用于显示内容
- 字符串用双引号 `"` 或单引号 `'` 包裹

### 1.2 基本计算

CNLab 可以当作计算器使用：

```matlab
# 基本算术运算
2 + 3       # 加法，结果为 5
10 - 4      # 减法，结果为 6
3 * 4       # 乘法，结果为 12
15 / 3      # 除法，结果为 5
2 ^ 3       # 幂运算，结果为 8（2的3次方）
```

**练习 1.1：** 计算 `(10 + 5) * 2 - 8 / 4` 的结果

---

## 第二章：变量与数据类型

### 2.1 什么是变量

变量是用来存储数据的容器。你可以把变量想象成一个贴有标签的盒子：

```matlab
# 创建变量
x = 5           # 把 5 存入名为 x 的变量
y = 3.14        # 把 3.14 存入名为 y 的变量
name = 'Alice'  # 把字符串存入 name 变量

# 使用变量
z = x + y       # z 的值为 8.14
```

**命名规则：**
- 必须以字母或下划线开头
- 可以包含字母、数字、下划线
- 区分大小写（`x` 和 `X` 是不同的变量）

```matlab
# 合法的变量名
age
user_name
_data
MatrixA

# 不合法的变量名
2ndValue      # 不能以数字开头
my-name       # 不能包含减号
```

### 2.2 数据类型

CNLab 有三种基本数据类型：

#### 标量（Scalar）

单个数值：

```matlab
integer = 42            # 整数
floating = 3.14159      # 浮点数
scientific = 6.02e23    # 科学计数法，表示 6.02 × 10²³
```

#### 字符串（String）

文本数据：

```matlab
str1 = 'Hello'
str2 = "World"
message = 'This is a test'
```

#### 布尔值（Boolean）

逻辑值，只有 `true`（真）或 `false`（假）：

```matlab
flag = true
empty = false
```

#### 元胞数组（Cell）

可以存储不同类型数据的数组：

```matlab
# 创建元胞数组
C = {1, "hello", [1, 2, 3]}

# 一维索引访问
C{1}        # 返回 1
C{2}        # 返回 "hello"

# 二维元胞数组
C2 = cell(2, 3)
C2{1,1} = "a"
C2{1,2} = "b"
C2{2,1} = 1
C2{2,2} = 2

# 二维索引访问
C2{1,1}     # 返回 "a"
C2{2,2}     # 返回 2
```

#### 结构体（Struct）

存储具有命名字段的数据：

```matlab
# 创建结构体
s.name = "Tom"
s.age = 25
s.score = 85.5

# 访问字段
disp(s.name)    # 输出: Tom
disp(s.age)     # 输出: 25

# 结构体数组
s(1).name = "Alice"
s(1).age = 30
s(2).name = "Bob"
s(2).age = 25
```

#### 表格（Table）

存储列式数据，类似数据库表：

```matlab
# 创建表格
T = table()
T.Name = ["Alice"; "Bob"; "Charlie"]
T.Age = [25; 30; 35]
T.Score = [85.5; 90.0; 78.5]

# 访问列
disp(T.Name)

# 行操作
h = height(T)           # 获取行数
w = width(T)            # 获取列数
T = addrow(T, "David", 28, 88.0)   # 添加行
row = getrow(T, 2)      # 获取第2行
T = removerow(T, 1)     # 删除第1行
```

#### 日期时间（DateTime）

表示日期和时间：

```matlab
# 创建日期时间
d = datetime("2024-01-15")
dt = datetime("2024-01-15 14:30:00")

# 与 Duration 运算
dt2 = dt + days(5)      # 加5天
dt3 = dt - hours(3)     # 减3小时
dur = dt2 - dt          # 两个日期相减得到 Duration
```

#### 时间间隔（Duration）

表示时间长度：

```matlab
# 创建 Duration
d1 = days(5)            # 5天
d2 = hours(3)           # 3小时
d3 = minutes(30)        # 30分钟
d4 = seconds(45)        # 45秒

# Duration 运算
d5 = d1 + d2            # 5天3小时
d6 = d1 - hours(12)     # 4天12小时
```

**练习 2.1：** 创建三个变量，分别存储你的年龄、姓名和一个布尔值表示是否喜欢编程

---

## 第三章：矩阵基础

### 3.1 什么是矩阵

矩阵是 CNLab 的核心数据类型。你可以把它看作一个数字表格：

```
      第1列  第2列  第3列
第1行 [ 1      2      3  ]
第2行 [ 4      5      6  ]
第3行 [ 7      8      9  ]
```

### 3.2 创建矩阵

#### 直接输入

使用方括号 `[]`，逗号分隔列，分号分隔行：

```matlab
# 行向量（1行3列）
v = [1, 2, 3]

# 列向量（3行1列）
w = [1; 2; 3]

# 2×3 矩阵（2行3列）
A = [1, 2, 3; 4, 5, 6]
```

#### 使用函数创建

```matlab
# 3×3 零矩阵
Z = zeros(3, 3)
# 结果:
# [0, 0, 0]
# [0, 0, 0]
# [0, 0, 0]

# 2×4 全1矩阵
O = ones(2, 4)
# 结果:
# [1, 1, 1, 1]
# [1, 1, 1, 1]

# 4×4 单位矩阵（对角线为1，其余为0）
I = eye(4)
# 结果:
# [1, 0, 0, 0]
# [0, 1, 0, 0]
# [0, 0, 1, 0]
# [0, 0, 0, 1]

# 随机矩阵
R = rand(3, 3)      # 0-1之间的均匀分布
N = randn(2, 2)     # 标准正态分布

# 随机整数矩阵
I = randi(10, 3, 3)     # 3×3矩阵，元素范围1-10
J = randi(5, 15, 2, 2)  # 2×2矩阵，元素范围5-15

# 线性等间距向量
v = linspace(0, 10, 5)   # [0, 2.5, 5, 7.5, 10]

# 2D网格坐标
x = [1, 2, 3]
y = [10, 20]
grid = meshgrid(x, y)   # 返回[X, Y]拼接矩阵

# 特殊矩阵
M = magic(3)        # 3阶魔方矩阵
H = hilb(4)         # 4阶Hilbert矩阵
P = pascal(5)       # 5阶Pascal矩阵
V = vander([1, 2, 3, 4])  # Vandermonde矩阵
```

### 3.3 访问矩阵元素

CNLab 使用 **1-based** 索引（第一个元素的索引是1，不是0）：

```matlab
A = [10, 20, 30; 40, 50, 60; 70, 80, 90]

# 访问单个元素
A(2, 3)     # 第2行第3列，结果为 60
A(1, 1)     # 第1行第1列，结果为 10

# 线性索引（按列优先）
A(1)        # 10（第1列第1行）
A(2)        # 40（第1列第2行）
A(5)        # 50（第2列第2行）

# 冒号索引（切片）
A(:, 1)     # 第1列的所有元素 [10; 40; 70]
A(1, :)     # 第1行的所有元素 [10, 20, 30]
A(:)        # 所有元素作为列向量

# 范围索引
v = [10, 20, 30, 40, 50, 60, 70, 80, 90]
v(2:5)      # 第2到第5个元素 [20; 30; 40; 50]
v(1:2:9)    # 从1到9，步长为2 [10; 30; 50; 70; 90]

# end 关键字（最后一个索引）
A(end)      # 最后一个元素（90）
A(end, 1)   # 最后一行第1列（70）
A(1, end)   # 第1行最后一列（30）

# 逻辑索引
A(A > 50)   # 返回所有大于50的元素 [60; 70; 80; 90]
```

### 3.4 修改矩阵元素

通过索引可以修改矩阵中的元素：

```matlab
A = [1, 2, 3; 4, 5, 6; 7, 8, 9]

# 二维索引赋值
A(2, 3) = 100     # 修改第2行第3列的值为 100

# 线性索引赋值（按列优先）
A(5) = 200        # 修改第5个元素（第2行第2列）为 200
```

**注意：** 索引必须在矩阵范围内，否则会产生错误。

### 3.5 创建序列

使用冒号 `:` 创建等差数列：

```matlab
# 语法: 起始值:步长:结束值
1:5         # [1, 2, 3, 4, 5]      默认步长为1
0:2:10      # [0, 2, 4, 6, 8, 10]  步长为2
10:-1:5     # [10, 9, 8, 7, 6, 5]  递减序列
0:0.5:2     # [0, 0.5, 1, 1.5, 2]  小数步长
```

**练习 3.1：** 创建一个 3×3 矩阵，包含数字 1 到 9

**练习 3.2：** 使用冒号语法创建序列 `[5, 10, 15, 20, 25]`

---

## 第四章：矩阵运算

### 4.1 基本运算

#### 加减法

对应元素相加减：

```matlab
A = [1, 2; 3, 4]
B = [5, 6; 7, 8]

C = A + B   # [6, 8; 10, 12]
D = B - A   # [4, 4; 4, 4]
```

#### 标量运算

矩阵与标量的加减乘除运算会自动广播到每个元素：

```matlab
A = [1, 2; 3, 4]

A + 10      # [11, 12; 13, 14]  每个元素加10
A - 5       # [-4, -3; -2, -1]  每个元素减5
A * 2       # [2, 4; 6, 8]      每个元素乘2
A / 2       # [0.5, 1; 1.5, 2]  每个元素除以2

# 标量在左边也支持
10 + A      # 与 A + 10 相同
5 - A       # [4, 3; 2, 1]      标量减每个元素
2 * A       # 与 A * 2 相同
```

#### 元素级运算

MATLAB 风格的元素级运算符：

```matlab
A = [1, 2, 3]
B = [4, 5, 6]

# 元素级乘法
A .* B      # [4, 10, 18]  对应元素相乘

# 元素级除法
A ./ B      # [0.25, 0.4, 0.5]  对应元素相除

# 元素级幂运算
A .^ 2      # [1, 4, 9]  每个元素平方
2 .^ A      # [2, 4, 8]  2的A次方

# 矩阵与标量的元素级运算
A .* 10     # [10, 20, 30]
```

#### 矩阵乘法

矩阵乘法遵循 "前行乘后列" 的规则：

```matlab
A = [1, 2; 3, 4]      # 2×2 矩阵
B = [5, 6; 7, 8]      # 2×2 矩阵

C = A * B
# 计算过程:
# C(1,1) = 1*5 + 2*7 = 19
# C(1,2) = 1*6 + 2*8 = 22
# C(2,1) = 3*5 + 4*7 = 43
# C(2,2) = 3*6 + 4*8 = 50
# 结果: [19, 22; 43, 50]
```

**重要：** 矩阵乘法要求第一个矩阵的列数等于第二个矩阵的行数。

```matlab
# 2×3 矩阵 × 3×2 矩阵 = 2×2 矩阵
A = [1, 2, 3; 4, 5, 6]    # 2行3列
B = [1, 2; 3, 4; 5, 6]    # 3行2列
C = A * B                 # 2行2列
```

#### 转置

将行变列，列变行：

```matlab
A = [1, 2, 3; 4, 5, 6]    # 2×3 矩阵
B = A'                    # 3×2 矩阵（共轭转置）
# B = [1, 4; 2, 5; 3, 6]

C = A.'                   # 3×2 矩阵（非共轭转置）
# 对于实数矩阵，' 和 .' 结果相同
# 对于复数矩阵，' 会取共轭，.' 不会
```

#### 幂运算

```matlab
A = [1, 2; 3, 4]
B = A ^ 2       # A * A

# 元素级幂运算
C = A .^ 2      # [1, 4; 9, 16]  每个元素平方
D = 2 .^ A      # [2, 4; 8, 16]  2的A次方
```

#### 矩阵左除（求解线性方程组）

```matlab
# 求解 Ax = b
A = [4, 7; 2, 6]
b = [5; 8]
x = A \ b       # 等价于 solve(A, b)

# 验证
A * x   # 应该等于 b
```

### 4.2 常用函数

```matlab
A = [1, 2, 3; 4, 5, 6; 7, 8, 9]

# 矩阵信息
size(A)         # 返回矩阵尺寸 [3, 3]

# 矩阵运算
det(A)          # 行列式
transpose(A)    # 转置（与 A' 相同）
inv(A)          # 逆矩阵（A必须是方阵且可逆）

# 统计函数
sum(A)          # 所有元素之和
mean(A)         # 平均值
max(A)          # 最大值
min(A)          # 最小值
```

**练习 4.1：** 创建两个 2×2 矩阵，计算它们的和、差、乘积

**练习 4.2：** 创建一个 3×3 矩阵，计算它的转置和行列式

---

## 第五章：控制流

### 5.1 条件判断（if语句）

程序需要根据不同条件执行不同代码：

```matlab
x = 5

if x > 0
    disp("x 是正数")
end
```

#### if-else 结构

```matlab
x = -3

if x > 0
    disp("x 是正数")
else
    disp("x 不是正数")
end
```

#### if-elseif-else 结构

```matlab
x = 0

if x > 0
    disp("x 是正数")
elseif x < 0
    disp("x 是负数")
else
    disp("x 是零")
end
```

### 5.2 比较运算符

```matlab
x = 5
y = 10

x == y      # 等于，false
x ~= y      # 不等于，true
x < y       # 小于，true
x > y       # 大于，false
x <= y      # 小于等于，true
x >= y      # 大于等于，false
```

#### 矩阵比较运算

比较运算符也支持矩阵与矩阵、矩阵与标量的比较，返回逻辑矩阵（0表示假，1表示真）：

```matlab
A = [1, 2, 3; 4, 5, 6]

# 矩阵与标量比较
A > 3       # [0, 0, 0; 1, 1, 1]  每个元素与3比较
A == 5      # [0, 0, 0; 0, 1, 0]  等于5的位置为1

# 矩阵与矩阵比较（维度必须相同）
B = [0, 2, 5; 4, 3, 6]
A > B       # [1, 0, 0; 0, 1, 0]  逐元素比较
```

### 5.3 逻辑运算符

```matlab
x = 5
y = 10

# 逻辑与：两个条件都成立
x > 0 && y > 0      # true（x和y都大于0）

# 逻辑或：至少一个条件成立
x > 10 || y > 5     # true（y大于5）

# 逻辑非：取反
~(x == 5)           # false（x等于5，取反后为false）
```

#### 元素级逻辑运算（用于矩阵）

```matlab
A = [1, 0, 1]
B = [1, 1, 0]

# 元素级逻辑与：对应元素都非零则结果为1
A & B             # [1, 0, 0]

# 元素级逻辑或：对应元素至少一个非零则结果为1
A | B             # [1, 1, 1]

# 示例：筛选矩阵中满足条件的元素
C = [1, 2, 3; 4, 5, 6]
mask = C > 3        # [0, 0, 0; 1, 1, 1]
```

### 5.4 for 循环

重复执行一段代码：

```matlab
# 基本语法
for i = 1:5
    disp(i)
end
# 输出: 1, 2, 3, 4, 5
```

```matlab
# 计算 1 到 100 的和
sum = 0
for i = 1:100
    sum = sum + i
end
disp(sum)       # 5050
```

```matlab
# 遍历小数步长
for x = 0:0.5:2
    y = sin(x)
    disp(y)
end
```

### 5.5 while 循环

当条件满足时持续循环：

```matlab
# 倒计时
n = 5
while n > 0
    disp(n)
    n = n - 1
end
disp("发射!")
```

```matlab
# 计算阶乘
n = 5
factorial = 1
while n > 1
    factorial = factorial * n
    n = n - 1
end
disp(factorial)     # 120
```

**练习 5.1：** 编写程序判断一个数是正数、负数还是零

**练习 5.2：** 使用 for 循环计算 1 到 10 的阶乘

**练习 5.3：** 使用 while 循环找出第一个大于 1000 的 2 的幂次

### 5.6 break 和 continue 语句

`break` 用于立即退出循环：

```matlab
# 找到第一个能被 7 整除的数
for n = 1:100
    if mod(n, 7) == 0
        disp(n)     # 输出 7
        break       # 退出循环
    end
end
```

`continue` 用于跳过当前迭代，继续下一次：

```matlab
# 输出 1-10 中的奇数
for n = 1:10
    if mod(n, 2) == 0
        continue    # 跳过偶数
    end
    disp(n)         # 只输出奇数: 1, 3, 5, 7, 9
end
```

---

## 第六章：函数

### 6.1 调用内置函数

CNLab 提供了丰富的内置函数：

```matlab
# 数学函数
sin(3.14159)        # 正弦
cos(0)              # 余弦
sqrt(16)            # 平方根
exp(1)              # 自然指数 e
log(10)             # 自然对数
log10(100)          # 常用对数
abs(-5)             # 绝对值

# 矩阵函数
zeros(3, 3)         # 零矩阵
ones(2, 2)          # 全1矩阵
eye(4)              # 单位矩阵
rand(3, 3)          # 随机矩阵

# 统计函数
sum([1, 2, 3, 4, 5])    # 求和
mean([1, 2, 3, 4, 5])   # 平均值
max([3, 1, 4, 1, 5])    # 最大值
min([3, 1, 4, 1, 5])    # 最小值
prod([2, 3, 4])         # 乘积，结果为 24
std([1, 2, 3, 4, 5])    # 标准差
var([1, 2, 3, 4, 5])    # 方差

# 取整函数
floor(3.7)              # 向下取整，结果为 3
ceil(3.2)               # 向上取整，结果为 4
round(3.5)              # 四舍五入，结果为 4
fix(-3.7)               # 向零取整，结果为 -3

# 符号和模运算
sign(-5)                # 符号函数，结果为 -1
mod(7, 3)               # 模运算，结果为 1
rem(7, 3)               # 余数，结果为 1
```

### 6.2 完整内置函数列表

#### 数学函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `sin(x)` | 正弦 | `sin(3.14159/2)` |
| `cos(x)` | 余弦 | `cos(0)` |
| `tan(x)` | 正切 | `tan(3.14159/4)` |
| `asin(x)` | 反正弦 | `asin(1)` |
| `acos(x)` | 反余弦 | `acos(1)` |
| `atan(x)` | 反正切 | `atan(1)` |
| `atan2(y, x)` | 四象限反正切 | `atan2(1, 1)` |
| `sinh(x)` | 双曲正弦 | `sinh(1)` |
| `cosh(x)` | 双曲余弦 | `cosh(1)` |
| `tanh(x)` | 双曲正切 | `tanh(1)` |
| `sqrt(x)` | 平方根 | `sqrt(16)` |
| `exp(x)` | 指数函数 | `exp(1)` |
| `log(x)` | 自然对数 | `log(10)` |
| `log10(x)` | 常用对数 | `log10(100)` |
| `log2(x)` | 以2为底对数 | `log2(8)` |
| `abs(x)` | 绝对值 | `abs(-5)` |
| `floor(x)` | 向下取整 | `floor(3.7)` |
| `ceil(x)` | 向上取整 | `ceil(3.2)` |
| `round(x)` | 四舍五入 | `round(3.5)` |
| `fix(x)` | 向零取整 | `fix(-3.7)` |
| `sign(x)` | 符号函数 | `sign(-5)` |
| `mod(x, y)` | 模运算（结果与y同号） | `mod(7, 3)` |
| `rem(x, y)` | 余数（结果与x同号） | `rem(7, 3)` |

#### 矩阵创建函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `zeros(m, n)` | m×n 零矩阵 | `zeros(3, 3)` |
| `ones(m, n)` | m×n 全1矩阵 | `ones(2, 2)` |
| `eye(n)` | n×n 单位矩阵 | `eye(4)` |
| `rand(m, n)` | 均匀分布随机矩阵 | `rand(3, 3)` |
| `randn(m, n)` | 正态分布随机矩阵 | `randn(2, 2)` |
| `linspace(a, b)` | 创建100个从a到b的等间距点 | `linspace(0, 10)` |
| `linspace(a, b, n)` | 创建n个从a到b的等间距点 | `linspace(0, 10, 50)` |
| `logspace(a, b)` | 创建50个从10^a到10^b的对数间距点 | `logspace(0, 2)` |
| `logspace(a, b, n)` | 创建n个从10^a到10^b的对数间距点 | `logspace(0, 2, 100)` |
| `meshgrid(x, y)` | 生成2D网格坐标 | `meshgrid(1:3, 1:4)` |
| `randi(imax, m, n)` | 随机整数矩阵 | `randi(10, 3, 3)` |
| `randi(imin, imax, m, n)` | 指定范围随机整数 | `randi(5, 15, 2, 2)` |
| `randperm(n)` | 1:n的随机排列 | `randperm(10)` |
| `randperm(n, k)` | 前k个随机排列 | `randperm(10, 5)` |
| `magic(n)` | n阶魔方矩阵 | `magic(3)` |
| `hilb(n)` | n阶Hilbert矩阵 | `hilb(4)` |
| `pascal(n)` | n阶Pascal矩阵 | `pascal(5)` |
| `vander(v)` | Vandermonde矩阵 | `vander([1, 2, 3])` |
| `blkdiag(A, B)` | 分块对角矩阵 | `blkdiag(eye(2), ones(2, 2))` |
| `kron(A, B)` | Kronecker积 | `kron(eye(2), ones(2, 2))` |

#### 数据类型函数

##### 元胞数组（Cell）

| 函数 | 说明 | 示例 |
|------|------|------|
| `cell()` | 创建空元胞数组 | `C = cell()` |
| `cell(n)` | 创建n个元素的一维元胞数组 | `C = cell(5)` |
| `cell(m, n)` | 创建m×n的二维元胞数组 | `C = cell(2, 3)` |
| `iscell(x)` | 判断是否为元胞数组 | `iscell(C)` |

##### 结构体（Struct）

| 函数 | 说明 | 示例 |
|------|------|------|
| `struct()` | 创建空结构体 | `s = struct()` |
| `isstruct(x)` | 判断是否为结构体 | `isstruct(s)` |
| `fieldnames(s)` | 获取所有字段名 | `fieldnames(s)` |

##### 表格（Table）

| 函数 | 说明 | 示例 |
|------|------|------|
| `table()` | 创建空表格 | `T = table()` |
| `istable(x)` | 判断是否为表格 | `istable(T)` |
| `height(T)` | 获取表格行数 | `height(T)` |
| `width(T)` | 获取表格列数 | `width(T)` |
| `addrow(T, ...)` | 添加一行数据 | `addrow(T, "name", 25)` |
| `getrow(T, i)` | 获取第i行 | `getrow(T, 2)` |
| `removerow(T, i)` | 删除第i行 | `removerow(T, 1)` |

##### 日期时间（DateTime）

| 函数 | 说明 | 示例 |
|------|------|------|
| `datetime(str)` | 从字符串创建 | `datetime("2024-01-15")` |
| `isdatetime(x)` | 判断是否为DateTime | `isdatetime(d)` |

##### 时间间隔（Duration）

| 函数 | 说明 | 示例 |
|------|------|------|
| `years(n)` | 创建n年的Duration | `years(2)` |
| `days(n)` | 创建n天的Duration | `days(5)` |
| `hours(n)` | 创建n小时的Duration | `hours(3)` |
| `minutes(n)` | 创建n分钟的Duration | `minutes(30)` |
| `seconds(n)` | 创建n秒的Duration | `seconds(45)` |
| `isduration(x)` | 判断是否为Duration | `isduration(d)` |

##### 分类数组（Categorical）

| 函数 | 说明 | 示例 |
|------|------|------|
| `categorical(cell)` | 从cell创建 | `categorical({"a", "b", "a"})` |
| `iscategorical(x)` | 判断是否为categorical | `iscategorical(c)` |
| `categories(c)` | 获取所有类别 | `categories(c)` |

#### 字符串处理函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `strcat(s1, s2, ...)` | 连接字符串 | `strcat('Hello', 'World')` |
| `strlength(str)` | 字符串长度 | `strlength('hello')` |
| `strcmp(s1, s2)` | 比较字符串 | `strcmp('abc', 'abc')` |
| `strcmpi(s1, s2)` | 忽略大小写比较 | `strcmpi('ABC', 'abc')` |
| `strfind(str, pattern)` | 查找子串位置 | `strfind('hello', 'll')` |
| `strrep(str, old, new)` | 替换子串 | `strrep('hello', 'll', 'xx')` |
| `upper(str)` | 转大写 | `upper('hello')` |
| `lower(str)` | 转小写 | `lower('HELLO')` |
| `strtrim(str)` | 去除首尾空白 | `strtrim('  hello  ')` |
| `num2str(num)` | 数字转字符串 | `num2str(123)` |
| `str2num(str)` | 字符串转数字 | `str2num('456')` |
| `split(str, delimiter)` | 分割字符串 | `split('a,b,c', ',')` |

#### 复数运算

| 函数 | 说明 | 示例 |
|------|------|------|
| `3+4i` 或 `3+4j` | 复数字面量 | `z = 3 + 4i` |
| `real(z)` | 实部 | `real(3+4i)` → `3` |
| `imag(z)` | 虚部 | `imag(3+4i)` → `4` |
| `abs(z)` | 模 | `abs(3+4i)` → `5` |
| `angle(z)` | 相位角 | `angle(3+4i)` |
| `conj(z)` | 共轭 | `conj(3+4i)` → `3-4i` |
| `complex(a, b)` | 创建复数 | `complex(3, 4)` → `3+4i` |

#### 矩阵运算函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `inv(A)` | 逆矩阵 | `inv([1, 2; 3, 4])` |
| `det(A)` | 行列式 | `det(A)` |
| `transpose(A)` | 转置 | `transpose(A)` |
| `size(A)` | 矩阵尺寸 | `size(A)` |
| `diag(A)` | 提取/创建对角矩阵 | `diag([1, 2, 3])` |
| `diag(A, k)` | 提取第k条对角线 | `diag(A, 1)` |
| `trace(A)` | 矩阵迹 | `trace(A)` |
| `tril(A)` | 下三角矩阵 | `tril(A)` |
| `tril(A, k)` | 第k条对角线以下的矩阵 | `tril(A, 1)` |
| `triu(A)` | 上三角矩阵 | `triu(A)` |
| `triu(A, k)` | 第k条对角线以上的矩阵 | `triu(A, -1)` |
| `horzcat(A, B)` | 水平拼接 | `horzcat(A, B)` |
| `vertcat(A, B)` | 垂直拼接 | `vertcat(A, B)` |
| `lu(A)` | LU分解 | `lu(A)` |
| `qr(A)` | QR分解 | `qr(A)` |
| `svd(A)` | SVD分解 | `svd(A)` |
| `eig(A)` | 特征值分解 | `eig(A)` |
| `chol(A)` | Cholesky分解 | `chol(A)` |
| `solve(A, b)` | 求解 Ax=b | `solve(A, b)` |

#### 统计函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `sum(A)` | 所有元素求和 | `sum([1, 2, 3])` |
| `sum(A, dim)` | 沿维度求和 | `sum(A, 1)` |
| `mean(A)` | 平均值 | `mean(A)` |
| `mean(A, dim)` | 沿维度平均 | `mean(A, 2)` |
| `max(A)` | 最大值 | `max(A)` |
| `max(A, dim)` | 沿维度最大 | `max(A, 1)` |
| `min(A)` | 最小值 | `min(A)` |
| `min(A, dim)` | 沿维度最小 | `min(A, 2)` |
| `prod(A)` | 所有元素乘积 | `prod([2, 3, 4])` |
| `prod(A, dim)` | 沿维度乘积 | `prod(A, 1)` |
| `std(A)` | 标准差 | `std(A)` |
| `rank(A)` | 矩阵秩 | `rank(A)` |
| `norm(A)` | 矩阵范数 | `norm(A)` |
| `cond(A)` | 矩阵条件数 | `cond(A)` |
| `length(A)` | 向量长度或矩阵最大维度 | `length(v)` |
| `numel(A)` | 元素总数 | `numel(A)` |
| `isempty(A)` | 判断是否为空矩阵 | `isempty([])` |

#### 矩阵操作函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `reshape(A, m, n)` | 重塑矩阵为 m×n | `reshape(v, 5, 1)` |
| `repmat(A, m, n)` | 重复拼接矩阵 | `repmat(A, 2, 2)` |
| `flipud(A)` | 上下翻转 | `flipud(A)` |
| `fliplr(A)` | 左右翻转 | `fliplr(A)` |
| `rot90(A)` | 逆时针旋转90度 | `rot90(A)` |
| `rot90(A, k)` | 逆时针旋转k×90度 | `rot90(A, 2)` |
| `sort(A)` | 排序 | `sort(v)` |
| `sort(A, dim)` | 沿维度排序 | `sort(A, 1)` |
| `find(A)` | 查找非零元素索引 | `find(v > 3)` |
| `unique(A)` | 去重 | `unique([1, 2, 2, 3])` |

#### 统计函数（扩展）

| 函数 | 说明 | 示例 |
|------|------|------|
| `std(A)` | 标准差 | `std(A)` |
| `std(A, dim)` | 沿维度标准差 | `std(A, 1)` |
| `var(A)` | 方差 | `var(A)` |
| `var(A, dim)` | 沿维度方差 | `var(A, 2)` |
| `cumsum(A)` | 累积和 | `cumsum(v)` |
| `cumsum(A, dim)` | 沿维度累积和 | `cumsum(A, 1)` |
| `cumprod(A)` | 累积积 | `cumprod(v)` |
| `cumprod(A, dim)` | 沿维度累积积 | `cumprod(A, 2)` |
| `diff(A)` | 差分 | `diff(v)` |
| `diff(A, n)` | n阶差分 | `diff(v, 2)` |

#### 输出函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `disp(x)` | 显示值并换行 | `disp("Hello")` |
| `print(x)` | 显示值并换行 | `print(A)` |
| `printf(x)` | 显示值不换行 | `printf("Loading...")` |

#### 文件I/O函数

##### CSV和分隔符文本

| 函数 | 说明 | 示例 |
|------|------|------|
| `csvread(filename)` | 读取CSV文件 | `data = csvread("data.csv")` |
| `csvread(filename, row, col)` | 从指定位置读取CSV | `data = csvread("data.csv", 1, 0)` |
| `csvwrite(filename, A)` | 写入CSV文件 | `csvwrite("output.csv", A)` |
| `dlmread(filename, delimiter)` | 读取分隔符文本 | `data = dlmread("data.txt", "\t")` |
| `dlmwrite(filename, A, delimiter)` | 写入分隔符文本 | `dlmwrite("output.txt", A, ",")` |
| `readmatrix(filename)` | 智能读取矩阵文件 | `data = readmatrix("data.csv")` |
| `writematrix(A, filename)` | 智能写入矩阵文件 | `writematrix(A, "output.csv")` |

##### 低级文件I/O

| 函数 | 说明 | 示例 |
|------|------|------|
| `fopen(filename, mode)` | 打开文件 | `fid = fopen("data.txt", "r")` |
| `fclose(fid)` | 关闭文件 | `fclose(fid)` |
| `fread(fid, size)` | 读取二进制数据 | `data = fread(fid, 100)` |
| `fwrite(fid, data)` | 写入二进制数据 | `fwrite(fid, A)` |
| `fprintf(fid, format, ...)` | 格式化写入 | `fprintf(fid, "%f\n", x)` |
| `fscanf(fid, format)` | 格式化读取 | `data = fscanf(fid, "%f")` |
| `sscanf(str, format)` | 从字符串读取 | `data = sscanf("1 2 3", "%f")` |
| `textscan(fid, format)` | 高级文本扫描 | `C = textscan(fid, "%s %f")` |

##### MAT文件操作

| 函数 | 说明 | 示例 |
|------|------|------|
| `save(filename, var1, ...)` | 保存变量到文件 | `save("data.mat", "A", "B")` |
| `load(filename)` | 从文件加载变量 | `load("data.mat")` |

##### 目录操作

| 函数 | 说明 | 示例 |
|------|------|------|
| `dir(path)` | 列出目录内容 | `files = dir(".")` |
| `mkdir(path)` | 创建目录 | `mkdir("new_folder")` |
| `rmdir(path)` | 删除目录 | `rmdir("old_folder")` |
| `pwd()` | 获取当前目录 | `current = pwd()` |
| `cd(path)` | 切换目录 | `cd("data/")` |

##### 路径操作

| 函数 | 说明 | 示例 |
|------|------|------|
| `fullfile(part1, part2, ...)` | 路径拼接 | `path = fullfile("data", "output.csv")` |
| `fileparts(filename)` | 路径分解 | `[p, name, ext] = fileparts("data/file.txt")` |
| `filesep()` | 获取文件分隔符 | `sep = filesep()` |

##### 文件操作

| 函数 | 说明 | 示例 |
|------|------|------|
| `exist(path)` | 检查路径是否存在 | `exist("file.txt")` |
| `isfile(path)` | 检查是否为文件 | `isfile("data.txt")` |
| `isfolder(path)` | 检查是否为目录 | `isfolder("data/")` |
| `deletefile(filename)` | 删除文件 | `deletefile("temp.txt")` |
| `copyfile(src, dst)` | 复制文件 | `copyfile("a.txt", "b.txt")` |
| `movefile(src, dst)` | 移动文件 | `movefile("a.txt", "dir/a.txt")` |

#### GPU函数

| 函数 | 说明 | 示例 |
|------|------|------|
| `enable_gpu()` | 启用GPU加速 | `enable_gpu()` |
| `disable_gpu()` | 禁用GPU加速 | `disable_gpu()` |
| `set_gpu_threshold(n)` | 设置GPU阈值 | `set_gpu_threshold(1000000)` |
| `gpu_info()` | 显示GPU信息 | `gpu_info()` |

### 6.3 函数调用规则

```matlab
# 无参数
I = eye(3)

# 单参数
s = sum(A)
d = det(A)

# 多参数
Z = zeros(3, 4)
R = rand(2, 3)

# 嵌套调用
result = sqrt(sum([4, 9, 16]))  # sqrt(29)
```

### 6.4 自定义函数

除了使用内置函数，你还可以创建自己的函数：

#### 函数定义语法

```matlab
function 输出变量 = 函数名(参数1, 参数2, ...)
    # 函数体
    输出变量 = 计算结果
end
```

#### 示例：单参数函数

```matlab
function y = square(x)
    y = x * x
end

# 调用函数
result = square(5)   # 结果为 25
```

#### 示例：多参数函数

```matlab
function z = add(x, y)
    z = x + y
end

# 调用函数
sum = add(3, 4)      # 结果为 7
```

#### 示例：无返回值函数

```matlab
function greet(name)
    disp("Hello, " + name)
end

# 调用函数
greet("CNLab")    # 输出: Hello, CNLab
```

**重要说明：**
- 函数定义必须在使用之前
- 函数内部定义的变量是局部变量，不影响全局作用域
- 函数通过输出变量返回值

---

## 第七章：线性代数

### 7.1 逆矩阵

如果 A × B = I（单位矩阵），则 B 是 A 的逆矩阵：

```matlab
A = [4, 7; 2, 6]
A_inv = inv(A)

# 验证
I = A * A_inv   # 结果接近单位矩阵 [1, 0; 0, 1]
```

### 7.2 求解线性方程组

求解 Ax = b：

```matlab
# 方程组:
# 2x + y = 11
# 5x + 7y = 13

A = [2, 1; 5, 7]
b = [11; 13]

# 方法1：使用逆矩阵
x = inv(A) * b

# 方法2：使用 solve 函数（推荐，更稳定）
x = solve(A, b)

# 方法3：使用左除运算符（MATLAB风格）
x = A \ b

# 验证
A * x   # 应该等于 b
```

### 7.3 矩阵分解

#### LU 分解

将矩阵分解为下三角矩阵 L 和上三角矩阵 U：

```matlab
A = [2, 1, -1; -3, -1, 2; -2, 1, 2]
lu_result = lu(A)

# lu_result.L 是下三角矩阵
# lu_result.U 是上三角矩阵
# 满足: A = L * U
```

#### QR 分解

将矩阵分解为正交矩阵 Q 和上三角矩阵 R：

```matlab
B = [1, -1; 1, 0; 1, 1]
qr_result = qr(B)

# qr_result.Q 是正交矩阵
# qr_result.R 是上三角矩阵
# 满足: B = Q * R
```

#### SVD 分解

奇异值分解：

```matlab
C = [4, 0; 0, 3; 0, 0]
svd_result = svd(C)

# svd_result.U 是左奇异向量矩阵
# svd_result.S 是奇异值对角矩阵
# svd_result.V 是右奇异向量矩阵
# 满足: C = U * S * V'
```

#### Eigen 分解（特征值分解）

求解实对称矩阵的特征值与特征向量：

```matlab
D = [2, -1, 0; -1, 2, -1; 0, -1, 2]
eig_result = eig(D)

# eig_result 返回一个拼接矩阵 [V; D_val]
# 其中上半部分 V 是特征向量矩阵
# 下半部分 D_val 是特征值对角矩阵
# 满足: D * V = V * D_val
```

#### Cholesky 分解

将对称正定矩阵分解为上三角矩阵 R，满足 A = R^T R：

```matlab
E = [4, 12, -16; 12, 37, -43; -16, -43, 98]
R = chol(E)

# R 是上三角矩阵
# 验证: E = R' * R
```

**练习 7.1**：求解方程组：
```
x + 2y + 3z = 14
2x + y + z = 7
3x + y + 2z = 11
```

---

## 第八章：GPU加速

### 8.1 启用 GPU 加速

对于大规模矩阵运算，可以使用 GPU 加速：

```matlab
# 启用 GPU 加速
enable_gpu()

# 查看 GPU 信息
gpu_info()
```

### 8.2 设置阈值

可以设置使用 GPU 的最小矩阵大小：

```matlab
# 只有矩阵元素超过 100 万时才使用 GPU
set_gpu_threshold(1000000)
```

### 8.3 自动调度

启用 GPU 后，大矩阵运算会自动调度到 GPU：

```matlab
enable_gpu()

# 创建大矩阵
A = rand(2000, 2000)
B = rand(2000, 2000)

# 自动使用 GPU 进行计算
C = A * B

disp("矩阵乘法完成")
disp(size(C))
```

### 8.4 禁用 GPU

```matlab
disable_gpu()
```

---

## 第九章：高级数据类型

### 9.1 元胞数组（Cell）

元胞数组可以存储不同类型的数据，使用 `{}` 创建和访问。

#### 创建元胞数组

```matlab
# 一维元胞数组
C = {1, "hello", [1, 2, 3], true}

# 二维元胞数组
C2 = cell(2, 3)
C2{1,1} = "a"
C2{1,2} = "b"
C2{1,3} = "c"
C2{2,1} = 1
C2{2,2} = 2
C2{2,3} = 3
```

#### 访问元胞数组

```matlab
# 一维索引
C{1}        # 返回 1
C{2}        # 返回 "hello"

# 二维索引
C2{1,1}     # 返回 "a"
C2{2,3}     # 返回 3
```

#### 相关函数

| 函数 | 说明 |
|------|------|
| `cell()` | 创建空元胞数组 |
| `cell(n)` | 创建n个元素的一维元胞数组 |
| `cell(m, n)` | 创建m×n的二维元胞数组 |
| `iscell(x)` | 判断是否为元胞数组 |

### 9.2 结构体（Struct）

结构体用于存储具有命名字段的数据。

#### 创建和访问结构体

```matlab
# 创建结构体
s.name = "Tom"
s.age = 25
s.score = 85.5

# 访问字段
disp(s.name)    # 输出: Tom
disp(s.age)     # 输出: 25

# 结构体数组
s(1).name = "Alice"
s(1).age = 30
s(2).name = "Bob"
s(2).age = 25

disp(s(1).name) # 输出: Alice
```

#### 相关函数

| 函数 | 说明 |
|------|------|
| `struct()` | 创建空结构体 |
| `isstruct(x)` | 判断是否为结构体 |
| `fieldnames(s)` | 获取所有字段名 |

### 9.3 表格（Table）

表格用于存储列式数据，类似数据库表。

#### 创建表格

```matlab
T = table()
T.Name = ["Alice"; "Bob"; "Charlie"]
T.Age = [25; 30; 35]
T.Score = [85.5; 90.0; 78.5]
```

#### 表格行操作

```matlab
# 获取表格尺寸
h = height(T)           # 返回行数
w = width(T)            # 返回列数

# 添加行
T = addrow(T, "David", 28, 88.0)

# 获取行
row = getrow(T, 2)      # 返回第2行作为cell数组

# 删除行
T = removerow(T, 1)     # 删除第1行
```

#### 相关函数

| 函数 | 说明 |
|------|------|
| `table()` | 创建空表格 |
| `istable(x)` | 判断是否为表格 |
| `height(T)` | 获取行数 |
| `width(T)` | 获取列数 |
| `addrow(T, ...)` | 添加一行数据 |
| `getrow(T, i)` | 获取第i行 |
| `removerow(T, i)` | 删除第i行 |

### 9.4 日期时间（DateTime）

DateTime 用于表示日期和时间。

#### 创建 DateTime

```matlab
d = datetime("2024-01-15")
dt = datetime("2024-01-15 14:30:00")
```

#### DateTime 与 Duration 运算

```matlab
dt = datetime("2024-01-15")
dur = days(5)

# DateTime + Duration = DateTime
dt2 = dt + dur          # 2024-01-20

# DateTime - Duration = DateTime
dt3 = dt - hours(12)    # 2024-01-14 12:00:00

# DateTime - DateTime = Duration
dt1 = datetime("2024-01-15")
dt2 = datetime("2024-01-20")
dur = dt2 - dt1         # 5 days
```

#### 相关函数

| 函数 | 说明 |
|------|------|
| `datetime(str)` | 从字符串创建 |
| `isdatetime(x)` | 判断是否为DateTime |

### 9.5 时间间隔（Duration）

Duration 表示时间长度。

#### 创建 Duration

```matlab
d1 = years(2)           # 2年
d2 = days(5)            # 5天
d3 = hours(3)           # 3小时
d4 = minutes(30)        # 30分钟
d5 = seconds(45)        # 45秒
```

#### Duration 运算

```matlab
d1 = days(5)
d2 = hours(12)

# Duration + Duration = Duration
d3 = d1 + d2            # 5.5 days

# Duration - Duration = Duration
d4 = d1 - hours(12)     # 4.5 days
```

#### 相关函数

| 函数 | 说明 |
|------|------|
| `years(n)` | 创建n年的Duration |
| `days(n)` | 创建n天的Duration |
| `hours(n)` | 创建n小时的Duration |
| `minutes(n)` | 创建n分钟的Duration |
| `seconds(n)` | 创建n秒的Duration |
| `isduration(x)` | 判断是否为Duration |

### 9.6 分类数组（Categorical）

Categorical 用于存储有限集合的离散值。

#### 创建和使用

```matlab
# 创建分类数组
C = categorical({"small", "medium", "large", "small", "medium"})

# 获取所有类别
cats = categories(C)    # {'small', 'medium', 'large'}
```

#### 相关函数

| 函数 | 说明 |
|------|------|
| `categorical(cell)` | 从cell创建 |
| `iscategorical(x)` | 判断是否为categorical |
| `categories(c)` | 获取所有类别 |

---

## 综合练习

### 练习 A：温度转换

编写程序将摄氏度转换为华氏度：
```
华氏度 = 摄氏度 × 9/5 + 32
```

### 练习 B：矩阵操作

创建一个 5×5 的随机矩阵，然后：
1. 提取对角线元素
2. 计算每行的平均值
3. 找出最大值及其位置

### 练习 C：数值积分

使用矩形法近似计算 sin(x) 在 [0, π] 上的积分：

```matlab
n = 1000                  # 分割数
h = 3.14159 / n           # 步长
sum = 0

for i = 1:n
    x = (i - 0.5) * h     # 中点
    sum = sum + sin(x)
end

integral = sum * h
disp(integral)            # 结果应接近 2
```

### 练习 D：斐波那契数列

编写程序生成前 20 个斐波那契数列：
```
F(1) = 1, F(2) = 1
F(n) = F(n-1) + F(n-2)
```

---

## 附录：运算符优先级

当表达式中有多个运算符时，按以下优先级计算（从高到低）：

| 优先级 | 运算符 | 说明 |
|--------|--------|------|
| 1 | `()` `[]` | 括号、矩阵构造、索引 |
| 2 | `'` `.'` | 转置（共轭/非共轭） |
| 3 | `^` `.^` | 幂运算（矩阵/元素级） |
| 4 | `+` `-` `~` | 一元正、负、非 |
| 5 | `*` `/` `\` `.*` `./` | 乘、除、左除、元素级乘除 |
| 6 | `+` `-` | 加、减 |
| 7 | `:` | 范围 |
| 8 | `<` `>` `<=` `>=` | 关系比较 |
| 9 | `==` `~=` | 相等比较 |
| 10 | `&` | 元素级逻辑与 |
| 11 | `\|` | 元素级逻辑或 |
| 12 | `&&` | 逻辑与（短路） |
| 13 | `\|\|` | 逻辑或（短路） |
| 14 | `=` | 赋值 |

**示例：**
```matlab
x = 2 + 3 * 4        # 先乘后加，结果为 14
y = (2 + 3) * 4      # 先加后乘，结果为 20
z = 2 ^ 3 + 1        # 先幂后加，结果为 9
```

---

## 附录：关键字列表

以下单词具有特殊含义，不能用作变量名：

| 关键字 | 说明 |
|--------|------|
| `if` | 条件判断 |
| `else` | 条件分支 |
| `elseif` | 多条件分支 |
| `for` | 循环 |
| `while` | 条件循环 |
| `function` | 函数定义 |
| `end` | 块结束标记 |
| `return` | 返回语句 |
| `break` | 跳出循环 |
| `continue` | 继续下一次循环 |
| `true` | 布尔真 |
| `false` | 布尔假 |
| `pi` | 圆周率 π |
| `eps` | 浮点精度 |
| `inf` | 无穷大 |
| `nan` | 非数字 |

---

## 附录：完整示例程序

### 示例1：求解线性方程组

```matlab
# 创建系数矩阵
A = [2, 1; 5, 7]

# 创建常数向量
b = [11; 13]

# 计算行列式
d = det(A)
disp("行列式:")
disp(d)

# 求逆矩阵
A_inv = inv(A)
disp("逆矩阵:")
disp(A_inv)

# 求解 x = A^(-1) * b
x = A_inv * b
disp("解:")
disp(x)

# 验证
result = A * x
disp("验证 A*x:")
disp(result)
```

### 示例2：矩阵分解

```matlab
# LU分解
A = [2, 1, -1; -3, -1, 2; -2, 1, 2]
lu_result = lu(A)

# QR分解
B = [1, -1; 1, 0; 1, 1]
qr_result = qr(B)

# SVD分解
C = [4, 0; 0, 3]
svd_result = svd(C)

# 特征值分解
D = [2, -1; -1, 2]
eig_result = eig(D)

# Cholesky分解
E = [4, 12; 12, 37]
chol_result = chol(E)

# 求解线性方程组
b = [8; -11; -3]
x = solve(A, b)
disp("方程组的解:")
disp(x)
```

### 示例3：GPU加速大矩阵运算

```matlab
# 启用GPU加速
enable_gpu()
set_gpu_threshold(1000000)  # 100万元素以上使用GPU

# 查看GPU信息
gpu_info()

# 大矩阵运算自动使用GPU
A = rand(2000, 2000)
B = rand(2000, 2000)
C = A * B  # 自动调度到GPU

disp("矩阵乘法完成")
disp(size(C))
```

---

## CNLab 与 MATLAB 语法对比分析

### 完全一致的特性

| 特性 | CNLab | MATLAB | 说明 |
|------|-------|--------|------|
| 矩阵构造 | `[1, 2; 3, 4]` | `[1, 2; 3, 4]` | 相同 |
| 矩阵索引 | `A(1, 2)`, `A(5)` | `A(1, 2)`, `A(5)` | 相同（1-based） |
| 转置 | `A'` | `A'` | 相同 |
| 算术运算 | `+ - * / ^` | `+ - * / ^` | 相同 |
| 比较运算 | `== ~= < > <= >=` | `== ~= < > <= >=` | 相同 |
| 逻辑运算 | `&& \|\| ~` | `&& \|\| ~` | 相同 |
| 元素级逻辑 | `& \|` | `& \|` | 相同 |
| 控制流 | `if/elseif/else/end` | `if/elseif/else/end` | 相同 |
| 循环 | `for/end`, `while/end` | `for/end`, `while/end` | 相同 |
| 函数定义 | `function y = f(x)` | `function y = f(x)` | 相同 |
| 内置函数 | `sin, cos, exp, log` 等 | `sin, cos, exp, log` 等 | 大部分相同 |
| 矩阵函数 | `zeros, ones, eye, rand` | `zeros, ones, eye, rand` | 相同 |
| 特殊矩阵 | `magic, hilb, pascal, vander` | `magic, hilb, pascal, vander` | 相同 |
| 随机函数 | `randi, randperm` | `randi, randperm` | 相同 |
| 网格生成 | `meshgrid` | `meshgrid` | 相同 |
| 分块矩阵 | `blkdiag, kron` | `blkdiag, kron` | 相同 |
| 线性代数 | `inv, det, lu, qr, svd, eig` | `inv, det, lu, qr, svd, eig` | 相同 |

### 主要差异

| 特性 | CNLab | MATLAB | 说明 |
|------|-------|--------|------|
| **单行注释** | `# 注释` | `% 注释` | **不同** |
| **多行注释** | `"""注释"""` | `%{ 注释 %}` | **不同** |
| 字符串 | `"text"` 或 `'text'` | `"text"` 或 `'text'` | 相同 |
| 范围语法 | `1:5`, `0:2:10` | `1:5`, `0:2:10` | 相同 |
| 矩阵拼接 | `horzcat/vertcat` | `[A, B]`, `[A; B]` | CNLab暂不支持 `[]` 拼接语法 |
| 取模运算 | `mod/rem` | `mod/rem` | 相同 |

### 总结

**与 MATLAB 的兼容性：约 95%**

CNLab 在核心语法和函数设计上与 MATLAB 高度一致，主要差异仅在于注释符号：
- CNLab 使用 `#` 和 `"""` 作为注释
- MATLAB 使用 `%` 和 `%{ %}` 作为注释

这使得熟悉 MATLAB 的用户可以几乎无缝地切换到 CNLab。

祝你学习愉快！
