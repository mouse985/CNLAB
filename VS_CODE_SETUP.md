# VS Code 配置指南

## 快速开始

### 1. 安装推荐扩展

打开 VS Code，按 `Ctrl+Shift+P`，输入 `Extensions: Show Recommended Extensions`，安装推荐的扩展：

- **MATLAB** (Gimly81.matlab) - 语法高亮和代码片段
- **C/C++ Extension Pack** - C++ 开发支持

### 2. 配置文件说明

项目已包含完整的 VS Code 配置：

```
.vscode/
├── tasks.json          # 运行任务配置
├── launch.json         # 调试配置
├── settings.json       # 编辑器设置
├── extensions.json     # 推荐扩展
└── snippets/
    └── matlab.json     # 代码片段
```

### 3. 运行脚本

#### 方式一：快捷键（推荐）

1. 打开 `.m` 文件
2. 按 `Ctrl+Shift+B` 运行当前脚本
3. 在终端面板查看结果

#### 方式二：命令面板

1. 按 `Ctrl+Shift+P`
2. 输入 `Run Task`
3. 选择 `Run CNLab Script`

#### 方式三：右键菜单

1. 在编辑器中右键点击
2. 选择 `Run Task` → `Run CNLab Script`

### 4. 进入交互模式

按 `Ctrl+Shift+P`，输入 `Run Task`，选择 `Run CNLab REPL`。

## 代码片段

输入以下前缀，按 `Tab` 键自动补全：

| 前缀 | 说明 |
|------|------|
| `function` | 函数定义 |
| `for` | for 循环 |
| `while` | while 循环 |
| `if` | if 语句 |
| `ifelse` | if-else 语句 |
| `matrix` | 创建矩阵 |
| `zeros` | 零矩阵 |
| `ones` | 全1矩阵 |
| `eye` | 单位矩阵 |
| `rand` | 随机矩阵 |
| `disp` | 显示输出 |
| `inv` | 矩阵求逆 |
| `transpose` | 矩阵转置 |

## 快捷键

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+Shift+B` | 运行当前脚本 |
| `Ctrl+Shift+P` | 命令面板 |
| `F5` | 调试运行 |
| `Ctrl+Space` | 触发代码补全 |

## 自定义配置

### 修改任务配置

编辑 `.vscode/tasks.json`：

```json
{
    "label": "Run CNLab Script",
    "command": "cnlab",  // 如果使用环境变量中的 cnlab
    "args": ["${file}"]
}
```

### 添加自定义代码片段

编辑 `.vscode/snippets/matlab.json`，添加新的代码片段：

```json
{
    "My Snippet": {
        "prefix": "mySnippet",
        "body": [
            "${1:code}"
        ],
        "description": "My custom snippet"
    }
}
```

## 故障排除

### 问题：无法找到 cnlab 命令

**解决方案：**

1. 确保已添加环境变量
2. 重启 VS Code
3. 或在 `tasks.json` 中使用完整路径：

```json
"command": "${workspaceFolder}/build/bin/Release/cnlab.exe"
```

### 问题：语法高亮不工作

**解决方案：**

1. 安装 MATLAB 扩展
2. 确保文件后缀为 `.m`
3. 检查 `.vscode/settings.json` 中的文件关联

### 问题：终端显示乱码

**解决方案：**

确保 `.vscode/settings.json` 中设置了正确的编码：

```json
{
    "files.encoding": "utf8"
}
```

## 高级配置

### 集成终端

VS Code 会自动使用集成终端运行脚本。可以在 `settings.json` 中配置终端：

```json
{
    "terminal.integrated.defaultProfile.windows": "PowerShell"
}
```

### 自动保存

建议开启自动保存：

```json
{
    "files.autoSave": "afterDelay",
    "files.autoSaveDelay": 1000
}
```

## 示例工作流

1. 创建新文件 `test.m`
2. 输入代码：
   ```matlab
   A = [1, 2; 3, 4]
   B = inv(A)
   disp("Done!")
   ```
3. 按 `Ctrl+Shift+B` 运行
4. 在终端查看结果

## 参考

- [VS Code 官方文档](https://code.visualstudio.com/docs)
- [MATLAB 扩展文档](https://marketplace.visualstudio.com/items?itemName=Gimly81.matlab)
