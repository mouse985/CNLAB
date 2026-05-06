/*
 * MatlabCN GPU Matrix Operations Implementation (Domestic Implementation)
 * Provides GPU-accelerated matrix operations, compatible with domestic GPUs
 */

#include "GPUMatrix.hpp"
#include "GPUCore.hpp"
#include "GPUKernel.hpp"
#include "GPUBuffer.hpp"
#include <fstream>
#include <sstream>

#ifdef ENABLE_OPENCL

namespace cnlab {
namespace gpu {

// Kernel source code embedded as strings
// In production, these would be loaded from files or embedded as binary resources

static const char* GEMM_KERNEL_SRC = R"(
#ifndef FLOAT_TYPE
#define FLOAT_TYPE float
#endif

#ifndef TILE_SIZE
#define TILE_SIZE 16
#endif

__kernel void gemm_basic(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int M,
    const int K,
    const int N)
{
    int row = get_global_id(0);
    int col = get_global_id(1);
    
    if (row < M && col < N) {
        FLOAT_TYPE sum = 0.0f;
        for (int k = 0; k < K; k++) {
            sum += A[row * K + k] * B[k * N + col];
        }
        C[row * N + col] = sum;
    }
}

__kernel void gemm_tiled(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int M,
    const int K,
    const int N)
{
    __local FLOAT_TYPE tileA[TILE_SIZE][TILE_SIZE];
    __local FLOAT_TYPE tileB[TILE_SIZE][TILE_SIZE];
    
    int localRow = get_local_id(0);
    int localCol = get_local_id(1);
    int globalRow = get_global_id(0);
    int globalCol = get_global_id(1);
    
    FLOAT_TYPE sum = 0.0f;
    
    int numTiles = (K + TILE_SIZE - 1) / TILE_SIZE;
    
    for (int t = 0; t < numTiles; t++) {
        int tiledRow = t * TILE_SIZE + localCol;
        int tiledCol = t * TILE_SIZE + localRow;
        
        if (globalRow < M && tiledRow < K) {
            tileA[localRow][localCol] = A[globalRow * K + tiledRow];
        } else {
            tileA[localRow][localCol] = 0.0f;
        }
        
        if (tiledCol < K && globalCol < N) {
            tileB[localRow][localCol] = B[tiledCol * N + globalCol];
        } else {
            tileB[localRow][localCol] = 0.0f;
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
        
        for (int k = 0; k < TILE_SIZE; k++) {
            sum += tileA[localRow][k] * tileB[k][localCol];
        }
        
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (globalRow < M && globalCol < N) {
        C[globalRow * N + globalCol] = sum;
    }
}
)";

static const char* MATRIX_OPS_KERNEL_SRC = R"(
#ifndef FLOAT_TYPE
#define FLOAT_TYPE float
#endif

__kernel void matrix_add(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        C[idx] = A[idx] + B[idx];
    }
}

__kernel void matrix_sub(
    __global const FLOAT_TYPE* A,
    __global const FLOAT_TYPE* B,
    __global FLOAT_TYPE* C,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        C[idx] = A[idx] - B[idx];
    }
}

__kernel void matrix_scale(
    __global const FLOAT_TYPE* A,
    __global FLOAT_TYPE* B,
    const FLOAT_TYPE alpha,
    const int rows,
    const int cols)
{
    int idx = get_global_id(0);
    int total = rows * cols;
    
    if (idx < total) {
        B[idx] = alpha * A[idx];
    }
}

__kernel void matrix_transpose(
    __global const FLOAT_TYPE* A,
    __global FLOAT_TYPE* B,
    const int rows,
    const int cols)
{
    int row = get_global_id(0);
    int col = get_global_id(1);
    
    if (row < rows && col < cols) {
        B[col * rows + row] = A[row * cols + col];
    }
}
)";

static const char* REDUCTION_KERNEL_SRC = R"(
#ifndef FLOAT_TYPE
#define FLOAT_TYPE float
#endif

#ifndef WG_SIZE
#define WG_SIZE 256
#endif

__kernel void reduction_sum(
    __global const FLOAT_TYPE* input,
    __global FLOAT_TYPE* output,
    __local FLOAT_TYPE* scratch,
    const int n)
{
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int wg_size = get_local_size(0);
    int wg_id = get_group_id(0);
    
    if (gid < n) {
        scratch[lid] = input[gid];
    } else {
        scratch[lid] = 0.0f;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (int offset = wg_size / 2; offset > 0; offset /= 2) {
        if (lid < offset) {
            scratch[lid] += scratch[lid + offset];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (lid == 0) {
        output[wg_id] = scratch[0];
    }
}

__kernel void reduction_max(
    __global const FLOAT_TYPE* input,
    __global FLOAT_TYPE* output,
    __local FLOAT_TYPE* scratch,
    const int n)
{
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int wg_size = get_local_size(0);
    int wg_id = get_group_id(0);
    
    if (gid < n) {
        scratch[lid] = input[gid];
    } else {
        scratch[lid] = -FLT_MAX;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (int offset = wg_size / 2; offset > 0; offset /= 2) {
        if (lid < offset) {
            scratch[lid] = max(scratch[lid], scratch[lid + offset]);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (lid == 0) {
        output[wg_id] = scratch[0];
    }
}

__kernel void reduction_min(
    __global const FLOAT_TYPE* input,
    __global FLOAT_TYPE* output,
    __local FLOAT_TYPE* scratch,
    const int n)
{
    int gid = get_global_id(0);
    int lid = get_local_id(0);
    int wg_size = get_local_size(0);
    int wg_id = get_group_id(0);
    
    if (gid < n) {
        scratch[lid] = input[gid];
    } else {
        scratch[lid] = FLT_MAX;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    for (int offset = wg_size / 2; offset > 0; offset /= 2) {
        if (lid < offset) {
            scratch[lid] = min(scratch[lid], scratch[lid + offset]);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    
    if (lid == 0) {
        output[wg_id] = scratch[0];
    }
}
)";

// Static kernel cache for compiled kernels
static std::unordered_map<std::string, std::unique_ptr<GPUKernel>> g_kernelCache;

static GPUKernel& getCachedKernel(const std::string& name, const std::string& source, const std::string& kernelFunc) {
    auto it = g_kernelCache.find(name);
    if (it != g_kernelCache.end()) {
        return *it->second;
    }
    
    auto kernel = std::make_unique<GPUKernel>(kernelFunc);
    kernel->compileFromSource(source);
    
    GPUKernel& ref = *kernel;
    g_kernelCache[name] = std::move(kernel);
    return ref;
}

GPUMatrix::GPUMatrix() = default;

GPUMatrix::GPUMatrix(size_t rows, size_t cols) : rows_(rows), cols_(cols) {
    buffer_.allocate(rows * cols);
}

GPUMatrix::GPUMatrix(const Matrix& hostMatrix) {
    upload(hostMatrix);
}

GPUMatrix::GPUMatrix(GPUMatrix&& other) noexcept
    : rows_(other.rows_)
    , cols_(other.cols_)
    , buffer_(std::move(other.buffer_)) {
    other.rows_ = 0;
    other.cols_ = 0;
}

GPUMatrix& GPUMatrix::operator=(GPUMatrix&& other) noexcept {
    if (this != &other) {
        rows_ = other.rows_;
        cols_ = other.cols_;
        buffer_ = std::move(other.buffer_);
        other.rows_ = 0;
        other.cols_ = 0;
    }
    return *this;
}

void GPUMatrix::upload(const Matrix& hostMatrix) {
    rows_ = hostMatrix.rows();
    cols_ = hostMatrix.cols();
    
    if (!buffer_.isValid() || buffer_.size() < rows_ * cols_) {
        buffer_.allocate(rows_ * cols_);
    }
    
    std::vector<float> data(rows_ * cols_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            data[i * cols_ + j] = static_cast<float>(hostMatrix(i, j));
        }
    }
    
    buffer_.upload(data.data(), data.size());
}

Matrix GPUMatrix::download() const {
    if (!buffer_.isValid()) {
        throw GPUException("GPU buffer not valid");
    }
    
    std::vector<float> data(rows_ * cols_);
    buffer_.download(data.data(), data.size());
    
    Matrix result(rows_, cols_);
    for (size_t i = 0; i < rows_; ++i) {
        for (size_t j = 0; j < cols_; ++j) {
            result(i, j) = static_cast<double>(data[i * cols_ + j]);
        }
    }
    
    return result;
}

GPUMatrix GPUMatrix::multiply(const GPUMatrix& A, const GPUMatrix& B) {
    if (A.cols_ != B.rows_) {
        throw GPUException("Matrix dimensions mismatch for multiplication");
    }
    
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    GPUMatrix C(A.rows_, B.cols_);
    
    size_t M = A.rows_;
    size_t K = A.cols_;
    size_t N = B.cols_;
    
    const size_t TILE_SIZE = 16;
    bool useTiled = (M >= TILE_SIZE && N >= TILE_SIZE && K >= TILE_SIZE);
    
    try {
        GPUKernel& kernel = useTiled 
            ? getCachedKernel("gemm_tiled", GEMM_KERNEL_SRC, "gemm_tiled")
            : getCachedKernel("gemm_basic", GEMM_KERNEL_SRC, "gemm_basic");
        
        kernel.setBufferArg(0, A.buffer_.getBuffer());
        kernel.setBufferArg(1, B.buffer_.getBuffer());
        kernel.setBufferArg(2, C.buffer_.getBuffer());
        kernel.setArg(3, static_cast<int>(M));
        kernel.setArg(4, static_cast<int>(K));
        kernel.setArg(5, static_cast<int>(N));
        
        if (useTiled) {
            cl::NDRange local(TILE_SIZE, TILE_SIZE);
            cl::NDRange global(
                ((M + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE,
                ((N + TILE_SIZE - 1) / TILE_SIZE) * TILE_SIZE
            );
            kernel.execute(global, local, true);
        } else {
            cl::NDRange global(M, N);
            kernel.execute(global, cl::NullRange, true);
        }
        
    } catch (const std::exception& e) {
        throw GPUException(std::string("GPU matrix multiplication failed: ") + e.what());
    }
    
    return C;
}

GPUMatrix GPUMatrix::add(const GPUMatrix& A, const GPUMatrix& B) {
    if (A.rows_ != B.rows_ || A.cols_ != B.cols_) {
        throw GPUException("Matrix dimensions mismatch for addition");
    }
    
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    GPUMatrix C(A.rows_, A.cols_);
    
    try {
        GPUKernel& kernel = getCachedKernel("matrix_add", MATRIX_OPS_KERNEL_SRC, "matrix_add");
        
        kernel.setBufferArg(0, A.buffer_.getBuffer());
        kernel.setBufferArg(1, B.buffer_.getBuffer());
        kernel.setBufferArg(2, C.buffer_.getBuffer());
        kernel.setArg(3, static_cast<int>(A.rows_));
        kernel.setArg(4, static_cast<int>(A.cols_));
        
        size_t total = A.rows_ * A.cols_;
        size_t wgSize = 256;
        cl::NDRange global(((total + wgSize - 1) / wgSize) * wgSize);
        cl::NDRange local(wgSize);
        
        kernel.execute(global, local, true);
        
    } catch (const std::exception& e) {
        throw GPUException(std::string("GPU matrix addition failed: ") + e.what());
    }
    
    return C;
}

GPUMatrix GPUMatrix::sub(const GPUMatrix& A, const GPUMatrix& B) {
    if (A.rows_ != B.rows_ || A.cols_ != B.cols_) {
        throw GPUException("Matrix dimensions mismatch for subtraction");
    }
    
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    GPUMatrix C(A.rows_, A.cols_);
    
    try {
        GPUKernel& kernel = getCachedKernel("matrix_sub", MATRIX_OPS_KERNEL_SRC, "matrix_sub");
        
        kernel.setBufferArg(0, A.buffer_.getBuffer());
        kernel.setBufferArg(1, B.buffer_.getBuffer());
        kernel.setBufferArg(2, C.buffer_.getBuffer());
        kernel.setArg(3, static_cast<int>(A.rows_));
        kernel.setArg(4, static_cast<int>(A.cols_));
        
        size_t total = A.rows_ * A.cols_;
        size_t wgSize = 256;
        cl::NDRange global(((total + wgSize - 1) / wgSize) * wgSize);
        cl::NDRange local(wgSize);
        
        kernel.execute(global, local, true);
        
    } catch (const std::exception& e) {
        throw GPUException(std::string("GPU matrix subtraction failed: ") + e.what());
    }
    
    return C;
}

GPUMatrix GPUMatrix::scale(const GPUMatrix& A, double alpha) {
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    GPUMatrix B(A.rows_, A.cols_);
    
    try {
        GPUKernel& kernel = getCachedKernel("matrix_scale", MATRIX_OPS_KERNEL_SRC, "matrix_scale");
        
        kernel.setBufferArg(0, A.buffer_.getBuffer());
        kernel.setBufferArg(1, B.buffer_.getBuffer());
        kernel.setArg(2, static_cast<float>(alpha));
        kernel.setArg(3, static_cast<int>(A.rows_));
        kernel.setArg(4, static_cast<int>(A.cols_));
        
        size_t total = A.rows_ * A.cols_;
        size_t wgSize = 256;
        cl::NDRange global(((total + wgSize - 1) / wgSize) * wgSize);
        cl::NDRange local(wgSize);
        
        kernel.execute(global, local, true);
        
    } catch (const std::exception& e) {
        throw GPUException(std::string("GPU matrix scaling failed: ") + e.what());
    }
    
    return B;
}

GPUMatrix GPUMatrix::transpose(const GPUMatrix& A) {
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    GPUMatrix B(A.cols_, A.rows_);
    
    try {
        GPUKernel& kernel = getCachedKernel("matrix_transpose", MATRIX_OPS_KERNEL_SRC, "matrix_transpose");
        
        kernel.setBufferArg(0, A.buffer_.getBuffer());
        kernel.setBufferArg(1, B.buffer_.getBuffer());
        kernel.setArg(2, static_cast<int>(A.rows_));
        kernel.setArg(3, static_cast<int>(A.cols_));
        
        cl::NDRange global(A.rows_, A.cols_);
        
        kernel.execute(global, cl::NullRange, true);
        
    } catch (const std::exception& e) {
        throw GPUException(std::string("GPU matrix transpose failed: ") + e.what());
    }
    
    return B;
}

double GPUMatrix::sum(const GPUMatrix& A) {
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    size_t total = A.rows_ * A.cols_;
    size_t wgSize = 256;
    size_t numGroups = (total + wgSize - 1) / wgSize;
    
    GPUBufferF tempBuffer(numGroups);
    
    try {
        GPUKernel& kernel = getCachedKernel("reduction_sum", REDUCTION_KERNEL_SRC, "reduction_sum");
        
        kernel.setBufferArg(0, A.buffer_.getBuffer());
        kernel.setBufferArg(1, tempBuffer.getBuffer());
        kernel.setLocalArg(2, wgSize * sizeof(float));
        kernel.setArg(3, static_cast<int>(total));
        
        cl::NDRange global(numGroups * wgSize);
        cl::NDRange local(wgSize);
        
        kernel.execute(global, local, true);
        
        std::vector<float> temp(numGroups);
        tempBuffer.download(temp.data(), temp.size());
        
        double result = 0.0;
        for (float val : temp) {
            result += val;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        throw GPUException(std::string("GPU matrix sum failed: ") + e.what());
    }
}

double GPUMatrix::max(const GPUMatrix& A) {
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    size_t total = A.rows_ * A.cols_;
    size_t wgSize = 256;
    size_t numGroups = (total + wgSize - 1) / wgSize;
    
    GPUBufferF tempBuffer(numGroups);
    
    try {
        GPUKernel& kernel = getCachedKernel("reduction_max", REDUCTION_KERNEL_SRC, "reduction_max");
        
        kernel.setBufferArg(0, A.buffer_.getBuffer());
        kernel.setBufferArg(1, tempBuffer.getBuffer());
        kernel.setLocalArg(2, wgSize * sizeof(float));
        kernel.setArg(3, static_cast<int>(total));
        
        cl::NDRange global(numGroups * wgSize);
        cl::NDRange local(wgSize);
        
        kernel.execute(global, local, true);
        
        std::vector<float> temp(numGroups);
        tempBuffer.download(temp.data(), temp.size());
        
        float result = -std::numeric_limits<float>::max();
        for (float val : temp) {
            result = std::max(result, val);
        }
        
        return static_cast<double>(result);
        
    } catch (const std::exception& e) {
        throw GPUException(std::string("GPU matrix max failed: ") + e.what());
    }
}

double GPUMatrix::min(const GPUMatrix& A) {
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    size_t total = A.rows_ * A.cols_;
    size_t wgSize = 256;
    size_t numGroups = (total + wgSize - 1) / wgSize;
    
    GPUBufferF tempBuffer(numGroups);
    
    try {
        GPUKernel& kernel = getCachedKernel("reduction_min", REDUCTION_KERNEL_SRC, "reduction_min");
        
        kernel.setBufferArg(0, A.buffer_.getBuffer());
        kernel.setBufferArg(1, tempBuffer.getBuffer());
        kernel.setLocalArg(2, wgSize * sizeof(float));
        kernel.setArg(3, static_cast<int>(total));
        
        cl::NDRange global(numGroups * wgSize);
        cl::NDRange local(wgSize);
        
        kernel.execute(global, local, true);
        
        std::vector<float> temp(numGroups);
        tempBuffer.download(temp.data(), temp.size());
        
        float result = std::numeric_limits<float>::max();
        for (float val : temp) {
            result = std::min(result, val);
        }
        
        return static_cast<double>(result);
        
    } catch (const std::exception& e) {
        throw GPUException(std::string("GPU matrix min failed: ") + e.what());
    }
}

GPUConfig& GPUConfig::getInstance() {
    static GPUConfig instance;
    return instance;
}

bool GPUConfig::shouldUseGPU(size_t elements) const {
    switch (policy_) {
        case GPUPolicy::FORCE_GPU:
            return true;
        case GPUPolicy::FORCE_CPU:
            return false;
        case GPUPolicy::AUTO:
        default:
            return elements >= threshold_;
    }
}

Matrix gpuMultiply(const Matrix& A, const Matrix& B) {
    GPUMatrix gA(A);
    GPUMatrix gB(B);
    GPUMatrix gC = GPUMatrix::multiply(gA, gB);
    return gC.download();
}

Matrix gpuAdd(const Matrix& A, const Matrix& B) {
    GPUMatrix gA(A);
    GPUMatrix gB(B);
    GPUMatrix gC = GPUMatrix::add(gA, gB);
    return gC.download();
}

Matrix gpuSub(const Matrix& A, const Matrix& B) {
    GPUMatrix gA(A);
    GPUMatrix gB(B);
    GPUMatrix gC = GPUMatrix::sub(gA, gB);
    return gC.download();
}

Matrix gpuScale(const Matrix& A, double alpha) {
    GPUMatrix gA(A);
    GPUMatrix gB = GPUMatrix::scale(gA, alpha);
    return gB.download();
}

Matrix gpuTranspose(const Matrix& A) {
    GPUMatrix gA(A);
    GPUMatrix gB = GPUMatrix::transpose(gA);
    return gB.download();
}

double gpuSum(const Matrix& A) {
    GPUMatrix gA(A);
    return GPUMatrix::sum(gA);
}

double gpuMax(const Matrix& A) {
    GPUMatrix gA(A);
    return GPUMatrix::max(gA);
}

double gpuMin(const Matrix& A) {
    GPUMatrix gA(A);
    return GPUMatrix::min(gA);
}

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
