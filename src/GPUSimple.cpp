/*
 * MatlabCN GPU Simple Implementation (Domestic/OpenCL)
 * Simplified OpenCL implementation for domestic GPU support
 */

#include "GPUSimple.hpp"
#include "GPUMatrix.hpp"
#include "GPUCore.hpp"
#include <iostream>
#include <sstream>

#ifdef ENABLE_OPENCL

namespace cnlab {
namespace gpu {

static bool g_gpuEnabled = false;
static size_t g_gpuThreshold = 1000000;
static bool g_autoMode = true;
static bool g_forceGPU = false;

class GPUContextImpl {
public:
    GPUContextImpl() : initialized_(false), autoMode_(true), threshold_(1000000), forceGPU_(false) {}
    
    bool initialize() {
        if (initialized_) {
            return true;
        }
        
        try {
            initialized_ = GPUCore::getInstance().initialize();
            return initialized_;
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("GPU initialization failed: ") + e.what());
        }
    }
    
    void shutdown() {
        if (initialized_) {
            GPUCore::getInstance().shutdown();
            initialized_ = false;
        }
    }
    
    bool isAvailable() const {
        return initialized_ && GPUCore::getInstance().isInitialized();
    }
    
    void setAutoMode(bool autoMode) { autoMode_ = autoMode; }
    void setThreshold(size_t minElements) { threshold_ = minElements; }
    void setForceGPU(bool force) { forceGPU_ = force; }
    
    bool shouldUseGPU(size_t matrixElements) const {
        if (forceGPU_) {
            return true;
        }
        if (!autoMode_) {
            return false;
        }
        return matrixElements >= threshold_;
    }
    
    std::string getInfo() const {
        if (!initialized_) {
            return "GPU not initialized";
        }
        return GPUCore::getInstance().getGPUInfo();
    }
    
private:
    bool initialized_;
    bool autoMode_;
    size_t threshold_;
    bool forceGPU_;
};

static GPUContextImpl& getImpl() {
    static GPUContextImpl impl;
    return impl;
}

GPUContext& GPUContext::getInstance() {
    static GPUContext instance;
    return instance;
}

GPUContext::GPUContext() 
    : initialized_(false)
    , autoMode_(true)
    , threshold_(1000000)
    , forceGPU_(false) {}

GPUContext::~GPUContext() {
    if (initialized_) {
        shutdown();
    }
}

bool GPUContext::initialize() {
    if (initialized_) {
        return true;
    }
    
    try {
        initialized_ = GPUCore::getInstance().initialize();
        g_gpuEnabled = initialized_;
        return initialized_;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("GPU initialization failed: ") + e.what());
    }
}

void GPUContext::shutdown() {
    if (initialized_) {
        GPUCore::getInstance().shutdown();
        initialized_ = false;
        g_gpuEnabled = false;
    }
}

bool GPUContext::isAvailable() const {
    return initialized_ && GPUCore::getInstance().isInitialized();
}

void GPUContext::setAutoMode(bool autoMode) {
    autoMode_ = autoMode;
    g_autoMode = autoMode;
}

void GPUContext::setThreshold(size_t minElements) {
    threshold_ = minElements;
    g_gpuThreshold = minElements;
}

void GPUContext::setForceGPU(bool force) {
    forceGPU_ = force;
    g_forceGPU = force;
}

bool GPUContext::shouldUseGPU(size_t matrixElements) const {
    if (forceGPU_) {
        return true;
    }
    if (!autoMode_) {
        return false;
    }
    return matrixElements >= threshold_;
}

std::string GPUContext::getInfo() const {
    if (!initialized_) {
        return "GPU not initialized";
    }
    return GPUCore::getInstance().getGPUInfo();
}

Matrix multiply(const Matrix& A, const Matrix& B) {
    if (!g_gpuEnabled) {
        throw std::runtime_error("GPU not enabled");
    }
    
    if (!GPUCore::getInstance().isInitialized()) {
        throw std::runtime_error("GPU not initialized");
    }
    
    return gpuMultiply(A, B);
}

Matrix add(const Matrix& A, const Matrix& B) {
    if (!g_gpuEnabled) {
        throw std::runtime_error("GPU not enabled");
    }
    
    if (!GPUCore::getInstance().isInitialized()) {
        throw std::runtime_error("GPU not initialized");
    }
    
    return gpuAdd(A, B);
}

Matrix transpose(const Matrix& A) {
    if (!g_gpuEnabled) {
        throw std::runtime_error("GPU not enabled");
    }
    
    if (!GPUCore::getInstance().isInitialized()) {
        throw std::runtime_error("GPU not initialized");
    }
    
    return gpuTranspose(A);
}

void enableGPU() {
    try {
        g_gpuEnabled = GPUCore::getInstance().initialize();
        if (!g_gpuEnabled) {
            throw std::runtime_error("Failed to enable GPU: initialization returned false");
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to enable GPU: ") + e.what());
    }
}

void disableGPU() {
    GPUCore::getInstance().shutdown();
    g_gpuEnabled = false;
}

void setGPUThreshold(size_t minElements) {
    g_gpuThreshold = minElements;
    GPUConfig::getInstance().setThreshold(minElements);
}

} // namespace gpu
} // namespace cnlab

#else // No OpenCL support

namespace cnlab {
namespace gpu {

GPUContext& GPUContext::getInstance() {
    static GPUContext instance;
    return instance;
}

GPUContext::GPUContext() 
    : initialized_(false)
    , autoMode_(true)
    , threshold_(1000000)
    , forceGPU_(false) {}

GPUContext::~GPUContext() = default;

bool GPUContext::initialize() {
    throw std::runtime_error("OpenCL support not compiled in");
}

void GPUContext::shutdown() {}

bool GPUContext::isAvailable() const { 
    return false; 
}

void GPUContext::setAutoMode(bool) {}
void GPUContext::setThreshold(size_t) {}
void GPUContext::setForceGPU(bool) {}

bool GPUContext::shouldUseGPU(size_t) const { 
    return false; 
}

std::string GPUContext::getInfo() const { 
    return "OpenCL not available"; 
}

Matrix multiply(const Matrix&, const Matrix&) {
    throw std::runtime_error("GPU not available - OpenCL support not compiled in");
}

Matrix add(const Matrix&, const Matrix&) {
    throw std::runtime_error("GPU not available - OpenCL support not compiled in");
}

Matrix transpose(const Matrix&) {
    throw std::runtime_error("GPU not available - OpenCL support not compiled in");
}

void enableGPU() {
    throw std::runtime_error("OpenCL support not compiled in");
}

void disableGPU() {}

void setGPUThreshold(size_t) {
    throw std::runtime_error("OpenCL support not compiled in");
}

std::string getGPUInfo() {
    return "OpenCL not available";
}

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
