/*
 * MatlabCN GPU Core Management (Domestic Implementation)
 * OpenCL platform/device management, compatible with domestic GPUs
 */

#pragma once

#ifdef ENABLE_OPENCL

#include <CL/cl.hpp>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace cnlab {
namespace gpu {

// GPU Exception class
class GPUException : public std::runtime_error {
public:
    explicit GPUException(const std::string& msg) : std::runtime_error(msg) {}
};

// GPU Device Info
struct GPUDeviceInfo {
    std::string name;
    std::string vendor;
    std::string version;
    cl_ulong globalMemory;
    cl_ulong localMemory;
    size_t maxWorkGroupSize;
    std::vector<size_t> maxWorkItemSizes;
    cl_uint computeUnits;
    bool isAvailable;
    
    bool isDomesticGPU() const;
};

// GPU Core Management Class
class GPUCore {
public:
    static GPUCore& getInstance();
    
    // Initialize/Shutdown
    bool initialize();
    void shutdown();
    bool isInitialized() const { return initialized_; }
    
    // Device enumeration and selection
    std::vector<GPUDeviceInfo> enumerateDevices();
    bool selectDevice(size_t index);
    bool selectDeviceByName(const std::string& name);
    bool selectDomesticGPU();
    
    // Get current device info
    GPUDeviceInfo getCurrentDeviceInfo() const;
    std::string getGPUInfo() const;
    
    // OpenCL object access
    cl::Context& getContext() { return context_; }
    cl::CommandQueue& getQueue() { return queue_; }
    cl::Device& getDevice() { return device_; }
    
    // Error handling
    static void checkError(cl_int err, const std::string& operation);
    
private:
    GPUCore() = default;
    ~GPUCore() { shutdown(); }
    
    GPUCore(const GPUCore&) = delete;
    GPUCore& operator=(const GPUCore&) = delete;
    
    bool initialized_ = false;
    cl::Platform platform_;
    cl::Device device_;
    cl::Context context_;
    cl::CommandQueue queue_;
    std::vector<cl::Device> devices_;
};

// Convenience functions
bool initializeGPU();
void shutdownGPU();
bool isGPUAvailable();
std::string getGPUInfo();

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
