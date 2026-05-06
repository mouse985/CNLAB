/*
 * MatlabCN GPU Buffer Management (Domestic Implementation)
 * OpenCL memory management, compatible with domestic GPUs
 */

#pragma once

#ifdef ENABLE_OPENCL

#include <CL/cl.hpp>
#include <cstddef>
#include "GPUCore.hpp"

namespace cnlab {
namespace gpu {

// GPU Buffer class
template<typename T>
class GPUBuffer {
public:
    GPUBuffer();
    GPUBuffer(size_t size, cl_mem_flags flags = CL_MEM_READ_WRITE);
    ~GPUBuffer();
    
    // Disable copy, enable move
    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;
    GPUBuffer(GPUBuffer&& other) noexcept;
    GPUBuffer& operator=(GPUBuffer&& other) noexcept;
    
    // Memory allocation
    void allocate(size_t size, cl_mem_flags flags = CL_MEM_READ_WRITE);
    void release();
    
    // Data transfer
    void upload(const T* hostData, size_t size, bool blocking = true);
    void download(T* hostData, size_t size, bool blocking = true) const;
    
    // Map access (zero copy)
    T* map(cl_map_flags flags, size_t size);
    void unmap(T* mappedPtr);
    
    // Fill data
    void fill(const T& pattern, size_t size);
    
    // Get OpenCL buffer
    cl::Buffer& getBuffer() { return buffer_; }
    const cl::Buffer& getBuffer() const { return buffer_; }
    
    // Get size
    size_t size() const { return size_; }
    bool isValid() const { return size_ > 0; }
    
private:
    cl::Buffer buffer_;
    size_t size_ = 0;
    cl_mem_flags flags_ = CL_MEM_READ_WRITE;
};

// Common type aliases
using GPUBufferF = GPUBuffer<float>;
using GPUBufferD = GPUBuffer<double>;

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
