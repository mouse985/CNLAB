/*
 * MatlabCN GPU Kernel Management Implementation (Domestic Implementation)
 * OpenCL kernel compilation and execution, compatible with domestic GPUs
 */

#include "GPUKernel.hpp"
#include "GPUCore.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

#ifdef ENABLE_OPENCL

namespace cnlab {
namespace gpu {

KernelCache& KernelCache::getInstance() {
    static KernelCache instance;
    return instance;
}

cl::Program KernelCache::getProgram(const std::string& key) {
    auto it = programs_.find(key);
    if (it == programs_.end()) {
        throw GPUException("Program not found in cache: " + key);
    }
    return it->second;
}

void KernelCache::storeProgram(const std::string& key, const cl::Program& program) {
    programs_[key] = program;
}

bool KernelCache::hasProgram(const std::string& key) const {
    return programs_.find(key) != programs_.end();
}

void KernelCache::clear() {
    programs_.clear();
}

GPUKernel::GPUKernel() = default;

GPUKernel::GPUKernel(const std::string& kernelName) : name_(kernelName) {}

GPUKernel::~GPUKernel() = default;

GPUKernel::GPUKernel(GPUKernel&& other) noexcept
    : name_(std::move(other.name_))
    , kernel_(std::move(other.kernel_))
    , program_(std::move(other.program_))
    , lastEvent_(std::move(other.lastEvent_))
    , compiled_(other.compiled_) {
    other.compiled_ = false;
}

GPUKernel& GPUKernel::operator=(GPUKernel&& other) noexcept {
    if (this != &other) {
        name_ = std::move(other.name_);
        kernel_ = std::move(other.kernel_);
        program_ = std::move(other.program_);
        lastEvent_ = std::move(other.lastEvent_);
        compiled_ = other.compiled_;
        other.compiled_ = false;
    }
    return *this;
}

void GPUKernel::compileFromSource(const std::string& source, const std::string& options) {
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    cl::Context& context = GPUCore::getInstance().getContext();
    cl::Device& device = GPUCore::getInstance().getDevice();
    
    program_ = cl::Program(context, source);
    
    cl_int err = program_.build({device}, options.c_str());
    if (err != CL_SUCCESS) {
        std::string buildLog = program_.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        throw GPUException("Kernel compilation failed: " + buildLog);
    }
    
    if (!name_.empty()) {
        kernel_ = cl::Kernel(program_, name_.c_str());
    }
    
    compiled_ = true;
}

void GPUKernel::compileFromFile(const std::string& filepath, const std::string& options) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw GPUException("Failed to open kernel file: " + filepath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    compileFromSource(buffer.str(), options);
}

void GPUKernel::setBufferArg(cl_uint index, const cl::Buffer& buffer) {
    if (!compiled_) {
        throw GPUException("Kernel not compiled");
    }
    
    cl_int err = kernel_.setArg(index, buffer);
    if (err != CL_SUCCESS) {
        throw GPUException("Failed to set buffer argument: " + std::to_string(err));
    }
}

void GPUKernel::setLocalArg(cl_uint index, size_t size) {
    if (!compiled_) {
        throw GPUException("Kernel not compiled");
    }
    
    cl_int err = kernel_.setArg(index, size, nullptr);
    if (err != CL_SUCCESS) {
        throw GPUException("Failed to set local argument: " + std::to_string(err));
    }
}

void GPUKernel::execute(const cl::NDRange& global, const cl::NDRange& local, bool blocking) {
    if (!compiled_) {
        throw GPUException("Kernel not compiled");
    }
    
    cl::CommandQueue& queue = GPUCore::getInstance().getQueue();
    cl_int err = queue.enqueueNDRangeKernel(
        kernel_,
        cl::NullRange,
        global,
        local,
        nullptr,
        &lastEvent_
    );
    
    if (err != CL_SUCCESS) {
        throw GPUException("Kernel execution failed: " + std::to_string(err));
    }
    
    if (blocking) {
        wait();
    }
}

void GPUKernel::wait() {
    if (lastEvent_()) {
        lastEvent_.wait();
    }
}

KernelManager& KernelManager::getInstance() {
    static KernelManager instance;
    return instance;
}

bool KernelManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    initialized_ = true;
    return true;
}

void KernelManager::shutdown() {
    kernels_.clear();
    initialized_ = false;
}

bool KernelManager::loadKernelFromFile(const std::string& name,
                                        const std::string& filepath,
                                        const std::string& kernelFunc) {
    if (!initialized_) {
        throw GPUException("KernelManager not initialized");
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw GPUException("Failed to open kernel file: " + filepath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    auto kernel = std::make_unique<GPUKernel>(kernelFunc);
    kernel->compileFromSource(buffer.str());
    
    kernels_[name] = std::move(kernel);
    return true;
}

GPUKernel& KernelManager::getKernel(const std::string& name) {
    auto it = kernels_.find(name);
    if (it == kernels_.end()) {
        throw GPUException("Kernel not found: " + name);
    }
    return *it->second;
}

GPUKernel& KernelManager::gemmBasic() {
    return getKernel("gemm_basic");
}

GPUKernel& KernelManager::gemmTiled() {
    return getKernel("gemm_tiled");
}

GPUKernel& KernelManager::matrixAdd() {
    return getKernel("matrix_add");
}

GPUKernel& KernelManager::matrixSub() {
    return getKernel("matrix_sub");
}

GPUKernel& KernelManager::matrixScale() {
    return getKernel("matrix_scale");
}

GPUKernel& KernelManager::matrixTranspose() {
    return getKernel("matrix_transpose");
}

GPUKernel& KernelManager::reductionSum() {
    return getKernel("reduction_sum");
}

GPUKernel& KernelManager::reductionMax() {
    return getKernel("reduction_max");
}

GPUKernel& KernelManager::reductionMin() {
    return getKernel("reduction_min");
}

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
