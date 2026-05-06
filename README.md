# CNLab - 国产科学计算平台 🇨🇳

<p align="center">
  <b>自主可控的 MATLAB-like 科学计算环境</b><br>
  从零实现矩阵运算引擎 | 脚本解释器 | 数据可视化 | GPU加速
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg" alt="C++17">
  <img src="https://img.shields.io/badge/CMake-3.14%2B-green.svg" alt="CMake">
  <img src="https://img.shields.io/badge/OpenCL-GPU%20加速-orange.svg" alt="OpenCL">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg" alt="Platform">
</p>

---

## ✨ 核心特性

- 🧮 **完整的矩阵运算系统** - 支持稠密矩阵、稀疏矩阵、各种分解算法
- 📝 **MATLAB兼容语法** - 支持.m脚本文件，熟悉的科学计算语法
- 📊 **数据可视化** - 2D绘图、多曲线、子图、散点图、柱状图
- 🚀 **GPU加速** - OpenCL支持，大矩阵运算自动调度到GPU
- 🔧 **零外部依赖** - 纯C++实现，自主可控
- 🌐 **跨平台** - Windows/Linux支持

---

## 🚀 快速开始

### 构建项目

```bash
# 克隆仓库
git clone https://github.com/mouse985/CNLAB.git
cd CNLAB

# 配置构建
cmake -B build -S .

# 编译（Release模式）
cmake --build build --config Release
```

### 运行方式

```bash
# 1. 交互模式 (REPL)
./build/bin/Release/cnlab

# 2. 运行脚本文件
./build/bin/Release/cnlab script.m

# 3. 执行单行代码
./build/bin/Release/cnlab -c "x = 5; y = x * 2; disp(y)"
```

---

## 📖 语法示例

### 矩阵运算

```matlab
# 创建矩阵
A = [1, 2, 3; 4, 5, 6; 7, 8, 9]
B = [1, 0; 0, 1]

# 特殊矩阵
Z = zeros(3, 3)
O = ones(2, 4)
I = eye(4)
R = rand(3, 3)

# 矩阵运算
C = A + B
D = A - B
E = A * B
F = A'           # 转置
G = inv(A)       # 逆矩阵
d = det(A)       # 行列式
H = A ^ 2        # 矩阵幂
```

### 范围表达式

```matlab
# 起始:步长:结束
t = 0:0.1:10
x = -10:0.01:10

# 默认步长为1
idx = 1:5
```

### 数据可视化

```matlab
# 单曲线绘图
x = 0:0.1:2*pi
y = sin(x)
plot(x, y)
show()

# 多曲线绘图（不同颜色）
x = -10:0.01:10
y = 3*x.^4 + 2*x.^3 + 7*x.^2 + 2*x + 9
g = 5*x.^3 + 9*x + 2
plot(x, y, 'r', x, g, 'g')  # 红色和绿色曲线
show()

# 子图
x = 0:0.01:5
y1 = exp(-1.5*x).*sin(10*x)
y2 = exp(-2*x).*sin(10*x)

subplot(1, 2, 1)
plot(x, y1)
title('Damped Oscillation 1')

subplot(1, 2, 2)
plot(x, y2)
title('Damped Oscillation 2')

show()
```

### 数学函数

```matlab
X = [1, 4; 9, 16]

# 基本函数
Y = sqrt(X)
Z = exp(X)
W = log(X)

# 三角函数
S = sin(X)
C = cos(X)
T = tan(X)

# 统计函数
M = max(X)
m = min(X)
s = sum(X)
a = mean(X)
```

### 控制流

```matlab
# if语句
if x > 0
    disp("positive")
elseif x < 0
    disp("negative")
else
    disp("zero")
end

# for循环
for i = 1:10
    disp(i)
end

# while循环
n = 5
while n > 0
    disp(n)
    n = n - 1
end
```

### 函数定义

```matlab
# 定义函数
function y = square(x)
    y = x * x
end

# 调用函数
result = square(5)
```

### GPU加速

```matlab
# 启用GPU
enable_gpu()

# 查看GPU信息
gpu_info()

# 大矩阵运算自动使用GPU
A = rand(2000, 2000)
B = rand(2000, 2000)
C = A * B  # 自动调度到GPU

# 禁用GPU
disable_gpu()
```

---

## 📁 项目结构

```
CNLAB/
├── include/           # 头文件
│   ├── Matrix.hpp     # 矩阵类
│   ├── LinearAlgebra.hpp  # 线性代数
│   ├── Lexer.hpp      # 词法分析器
│   ├── Parser.hpp     # 语法分析器
│   ├── Evaluator.hpp  # 执行引擎
│   └── Interpreter.hpp # 解释器
├── src/               # 源码
│   ├── Matrix.cpp     # 矩阵运算
│   ├── LinearAlgebra.cpp  # 线性代数实现
│   ├── Lexer.cpp      # 词法分析
│   ├── Parser.cpp     # 语法分析
│   ├── Evaluator.cpp  # 表达式求值
│   ├── Interpreter.cpp # 脚本执行
│   ├── Plot/          # 绘图模块
│   └── main.cpp       # 主程序
├── tests/             # 测试脚本
├── examples/          # 示例脚本
└── CMakeLists.txt     # 构建配置
```

---

## ✅ 功能清单

### 矩阵运算
- [x] 基础运算 (+, -, *, /, ^)
- [x] 转置、逆矩阵
- [x] 行列式、范数
- [x] 矩阵分解 (LU, QR, SVD)
- [x] 特征值、特征向量
- [x] 稀疏矩阵支持

### 数学函数
- [x] 三角函数 (sin, cos, tan, asin, acos, atan)
- [x] 双曲函数 (sinh, cosh, tanh)
- [x] 指数对数 (exp, log, log10, sqrt)
- [x] 统计函数 (sum, mean, max, min, prod)

### 数据可视化
- [x] 2D线图 (plot)
- [x] 多曲线绘图
- [x] 子图支持 (subplot)
- [x] 散点图 (scatter)
- [x] 柱状图 (bar)
- [x] 直方图 (hist)

### 语言特性
- [x] 变量赋值
- [x] 范围表达式 (a:step:b)
- [x] 矩阵字面量
- [x] 控制流 (if/else, for, while)
- [x] 函数定义与调用
- [x] 逗号/分号分隔符

### 高级特性
- [x] GPU加速 (OpenCL)
- [x] 内存池优化
- [x] 文件I/O (.mat格式)

---

## 🔧 C++ API

```cpp
#include "Matrix.hpp"
#include "Interpreter.hpp"

// 直接使用矩阵类
cnlab::Matrix A = cnlab::Matrix::zeros(3, 3);
cnlab::Matrix B = cnlab::Matrix::rand(3, 3);
cnlab::Matrix C = A + B;
cnlab::Matrix D = C.inverse();

C.print("Matrix C");

// 使用解释器执行脚本
cnlab::Interpreter interpreter;
interpreter.run("x = 5; y = x * 2");
interpreter.runFile("script.m");
```

---

## 🖥️ 系统要求

- **操作系统**: Windows 10+ / Linux
- **编译器**: MSVC 2019+ / GCC 9+ / Clang 10+
- **CMake**: 3.14+
- **可选**: OpenCL 3.0+ (用于GPU加速)

---

## 🤝 贡献指南

欢迎提交Issue和PR！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开 Pull Request

---

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 🙏 致谢

- 感谢所有贡献者的辛勤工作
- 感谢开源社区的启发和支持

---

<p align="center">
  <b>Made with ❤️ in China</b><br>
  <i>为国产科学计算软件生态贡献力量</i>
</p>
