/*
 * MatlabCN GPU Matrix Operations Interface (Domestic Implementation)
 * Provides GPU-accelerated version of Matrix class, compatible with domestic GPUs
 */

#pragma once

#ifdef ENABLE_OPENCL

#include "Matrix.hpp"
#include "GPUBuffer.hpp"
#include <memory>

namespace cnlab {
namespace gpu {

// Forward declaration
class GPUKernel;

// GPU Matrix class - wraps Matrix GPU operations
class GPUMatrix {
public:
    // Constructors
    GPUMatrix();
    explicit GPUMatrix(size_t rows, size_t cols);
    explicit GPUMatrix(const Matrix& hostMatrix);
    
    // Disable copy, enable move
    GPUMatrix(const GPUMatrix&) = delete;
    GPUMatrix& operator=(const GPUMatrix&) = delete;
    GPUMatrix(GPUMatrix&& other) noexcept;
    GPUMatrix& operator=(GPUMatrix&& other) noexcept;
    
    // Data transfer
    void upload(const Matrix& hostMatrix);
    Matrix download() const;
    
    // Matrix operations
    static GPUMatrix multiply(const GPUMatrix& A, const GPUMatrix& B);
    static GPUMatrix add(const GPUMatrix& A, const GPUMatrix& B);
    static GPUMatrix sub(const GPUMatrix& A, const GPUMatrix& B);
    static GPUMatrix scale(const GPUMatrix& A, double alpha);
    static GPUMatrix transpose(const GPUMatrix& A);
    
    // Reduction operations
    static double sum(const GPUMatrix& A);
    static double max(const GPUMatrix& A);
    static double min(const GPUMatrix& A);
    
    // Get info
    size_t rows() const { return rows_; }
    size_t cols() const { return cols_; }
    size_t size() const { return rows_ * cols_; }
    
    // Get GPU buffer
    GPUBufferF& buffer() { return buffer_; }
    const GPUBufferF& buffer() const { return buffer_; }
    
private:
    size_t rows_ = 0;
    size_t cols_ = 0;
    GPUBufferF buffer_;
};

// GPU scheduling policy
enum class GPUPolicy {
    AUTO = 0,
    FORCE_GPU = 1,
    FORCE_CPU = 2
};

// Global GPU configuration
class GPUConfig {
public:
    static GPUConfig& getInstance();
    
    // Threshold configuration
    void setThreshold(size_t elements) { threshold_ = elements; }
    size_t getThreshold() const { return threshold_; }
    
    // Policy configuration
    void setPolicy(GPUPolicy policy) { policy_ = policy; }
    GPUPolicy getPolicy() const { return policy_; }
    
    // Check if should use GPU
    bool shouldUseGPU(size_t elements) const;
    
private:
    GPUConfig() = default;
    
    size_t threshold_ = 1000000;
    GPUPolicy policy_ = GPUPolicy::AUTO;
};

// Convenience functions
Matrix gpuMultiply(const Matrix& A, const Matrix& B);
Matrix gpuAdd(const Matrix& A, const Matrix& B);
Matrix gpuSub(const Matrix& A, const Matrix& B);
Matrix gpuScale(const Matrix& A, double alpha);
Matrix gpuTranspose(const Matrix& A);

double gpuSum(const Matrix& A);
double gpuMax(const Matrix& A);
double gpuMin(const Matrix& A);

// Enable/Disable GPU
void enableGPU();
void disableGPU();
bool isGPUEnabled();
void setGPUThreshold(size_t elements);

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
