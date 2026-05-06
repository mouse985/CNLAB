/*
 * MatlabCN GPU Kernel Management (Domestic Implementation)
 * OpenCL kernel compilation and execution, compatible with domestic GPUs
 */

#pragma once

#ifdef ENABLE_OPENCL

#include "GPUCore.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace cnlab {
namespace gpu {

// Kernel compilation cache
class KernelCache {
public:
    static KernelCache& getInstance();
    
    cl::Program getProgram(const std::string& key);
    void storeProgram(const std::string& key, const cl::Program& program);
    bool hasProgram(const std::string& key) const;
    void clear();
    
private:
    KernelCache() = default;
    std::unordered_map<std::string, cl::Program> programs_;
};

// GPU Kernel class
class GPUKernel {
public:
    GPUKernel();
    explicit GPUKernel(const std::string& kernelName);
    ~GPUKernel();
    
    // Disable copy, enable move
    GPUKernel(const GPUKernel&) = delete;
    GPUKernel& operator=(const GPUKernel&) = delete;
    GPUKernel(GPUKernel&& other) noexcept;
    GPUKernel& operator=(GPUKernel&& other) noexcept;
    
    // Compile kernel from source
    void compileFromSource(const std::string& source, 
                           const std::string& options = "");
    
    // Load and compile kernel from file
    void compileFromFile(const std::string& filepath,
                         const std::string& options = "");
    
    // Set kernel arguments
    template<typename T>
    void setArg(cl_uint index, const T& value) {
        if (!compiled_) {
            throw GPUException("Kernel not compiled");
        }
        cl_int err = kernel_.setArg(index, value);
        if (err != CL_SUCCESS) {
            throw GPUException("Failed to set arg: error code " + std::to_string(err));
        }
    }
    
    void setBufferArg(cl_uint index, const cl::Buffer& buffer);
    void setLocalArg(cl_uint index, size_t size);
    
    // Execute kernel
    void execute(const cl::NDRange& global,
                 const cl::NDRange& local = cl::NullRange,
                 bool blocking = false);
    
    // Wait for completion
    void wait();
    
    // Get kernel object
    cl::Kernel& getKernel() { return kernel_; }
    
    // Get kernel info
    std::string getName() const { return name_; }
    
private:
    std::string name_;
    cl::Kernel kernel_;
    cl::Program program_;
    cl::Event lastEvent_;
    bool compiled_ = false;
};

// Kernel manager - auto-load common kernels
class KernelManager {
public:
    static KernelManager& getInstance();
    
    bool initialize();
    void shutdown();
    
    // Get pre-compiled kernel
    GPUKernel& getKernel(const std::string& name);
    
    // Common kernel shortcuts
    GPUKernel& gemmBasic();
    GPUKernel& gemmTiled();
    GPUKernel& matrixAdd();
    GPUKernel& matrixSub();
    GPUKernel& matrixScale();
    GPUKernel& matrixTranspose();
    GPUKernel& reductionSum();
    GPUKernel& reductionMax();
    GPUKernel& reductionMin();
    
private:
    KernelManager() = default;
    
    bool loadKernelFromFile(const std::string& name,
                            const std::string& filepath,
                            const std::string& kernelFunc);
    
    std::unordered_map<std::string, std::unique_ptr<GPUKernel>> kernels_;
    bool initialized_ = false;
};

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
