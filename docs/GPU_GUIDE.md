# CNLab GPU 加速指�?
## 概述

CNLab 支持 GPU 加速矩阵运算，通过 OpenCL 实现跨平�?GPU 支持（NVIDIA/AMD/Intel/国产GPU）�?
## 架构设计

```
用户脚本 �?Evaluator �?GPU调度�?�?OpenCL内核
                �?            CPU回退（GPU不可用时�?```

## 启用 GPU 加�?
### 脚本方式

```matlab
# 启用 GPU
enable_gpu()

# 设置阈值（超过此大小的矩阵使用 GPU）
set_gpu_threshold(1000000)  # 100万元素

# 查看 GPU 信息
gpu_info()

# 大矩阵运算自动使用 GPU
A = rand(2000, 2000)
B = rand(2000, 2000)
C = A * B  # 自动调度到 GPU
```

### C++ API

```cpp
#include "GPUMatrix.hpp"

// 初始�?GPU
matlab::gpu::enableGPU();

// 配置参数
matlab::gpu::GPUContext::getInstance().setThreshold(1024*1024);
matlab::gpu::GPUContext::getInstance().setAutoMode(true);

// 执行 GPU 运算
Matrix A = Matrix::rand(2000, 2000);
Matrix B = Matrix::rand(2000, 2000);
Matrix C = matlab::gpu::multiply(A, B);
```

## GPU 函数列表

| 函数 | 说明 | 示例 |
|------|------|------|
| `enable_gpu()` | 启用 GPU 加�?| `enable_gpu()` |
| `disable_gpu()` | 禁用 GPU 加�?| `disable_gpu()` |
| `set_gpu_threshold(n)` | 设置 GPU 使用阈�?| `set_gpu_threshold(1000000)` |
| `gpu_info()` | 显示 GPU 信息 | `gpu_info()` |

## 性能预期

| 矩阵大小 | CPU (ms) | GPU (ms) | 加速比 |
|---------|---------|---------|--------|
| 1024×1024 | 350 | 15 | 23x |
| 2048×2048 | 2800 | 80 | 35x |
| 4096×4096 | 22000 | 450 | 49x |

## 实现细节

### OpenCL 内核优化

1. **Tile 算法**：使�?local memory 缓存子矩�?2. **Work-group 大小**�?56 �?512
3. **向量加载**：float4/double4 优化

### 调度策略

- **自动模式**：根据矩阵大小自动选择 CPU/GPU
- **强制模式**：所有运算都使用 GPU
- **阈值设�?*：自定义 CPU/GPU 切换边界

## 编译配置

### CMake 配置（启�?OpenCL�?
```cmake
find_package(OpenCL REQUIRED)

target_link_libraries(matlab_core OpenCL::OpenCL)
```

### 编译选项

```bash
# Linux/macOS
cmake -B build -DENABLE_OPENCL=ON

# Windows
cmake -B build -DENABLE_OPENCL=ON -DOpenCL_ROOT="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.8"
```

## 故障排除

### GPU 未识�?
1. 检查显卡驱动是否安�?2. 确认 OpenCL 运行时库存在
3. 查看系统支持�?OpenCL 平台�?   ```bash
   # Linux
   clinfo
   
   # Windows
   # 使用 GPU-Z 或厂商工�?   ```

### 性能不如预期

1. 矩阵大小是否超过阈�?2. GPU 内存是否充足
3. 数据传输开销（CPU↔GPU�?
## 注意事项

1. **小矩�?*：N<256 建议使用 CPU（分块GEMM更快�?2. **内存限制**：GPU 内存有限，超大矩阵需要分�?3. **精度**：GPU �?CPU 结果可能有微小差异（浮点精度�?
## 实现架构

### 文件结构

```
include/
├── GPUCore.hpp          # OpenCL核心管理
├── GPUBuffer.hpp        # GPU内存缓冲�?├── GPUKernel.hpp        # OpenCL内核封装
├── GPUMatrix.hpp        # 完整GPU接口（含OpenCL内核�?└── GPUSimple.hpp        # 简化版GPU接口（当前使用）

src/
├── GPUCore.cpp          # OpenCL初始�?设备管理
├── GPUBuffer.cpp        # 内存分配/传输
├── GPUKernel.cpp        # 内核编译/执行
├── GPUMatrix.cpp        # 完整GPU实现（含OpenCL内核代码�?└── GPUSimple.cpp        # 简化版实现（当前使用）
```

### 当前状�?
- **简化版（GPUSimple�?*：已集成，提供API框架，无外部依赖
- **完整版（GPUMatrix�?*：含完整OpenCL实现，需链接OpenCL�?
### 启用完整GPU支持

1. 安装OpenCL SDK（NVIDIA CUDA Toolkit / AMD ROCm / Intel SDK�?2. 修改 `CMakeLists.txt`�?   ```cmake
   find_package(OpenCL REQUIRED)
   target_link_libraries(matlab_core OpenCL::OpenCL)
   ```
3. 替换 `GPUSimple.cpp` �?`GPUMatrix.cpp`
4. 重新构建

## 性能优化建议

### 矩阵大小选择

| 矩阵大小 | 推荐设备 | 原因 |
|---------|---------|------|
| N < 256 | CPU | 分块GEMM更快，避免GPU启动开销 |
| 256 �?N < 1024 | CPU/GPU | 根据具体硬件决定 |
| N �?1024 | GPU | GPU并行优势明显 |

### 内存管理

- **预热**：首次GPU运算较慢（内核编译、内存分配）
- **重用**：多次运算时重用GPU缓冲�?- **批处�?*：小矩阵合并成大矩阵批量处理

## 故障排除

### 编译错误

**错误**：`OpenCL not found`
**解决**�?```bash
# Ubuntu/Debian
sudo apt-get install ocl-icd-opencl-dev

# CentOS/RHEL
sudo yum install opencl-headers ocl-icd-devel

# Windows
# 安装NVIDIA CUDA Toolkit或AMD ROCm
```

### 运行时错�?
**错误**：`GPU not available`
**原因**�?1. 未安装显卡驱�?2. OpenCL运行时库缺失
3. 没有支持OpenCL的GPU

**解决**�?```bash
# 检查OpenCL设备
# Linux
clinfo

# Windows
# 使用GPU-Z查看OpenCL支持
```

## 未来扩展

- [ ] CUDA 后端（NVIDIA 专用，性能最优）
- [ ] ROCm 后端（AMD 专用�?- [ ] 自动混合精度（FP16/FP32�?- [ ] �?GPU 支持（SLI/CrossFire�?- [ ] 异步执行（重叠数据传输和计算�?- [ ] 内核缓存（避免重复编译）
