/*
 * MatlabCN GPU Core Management Implementation (Domestic Implementation)
 * OpenCL platform/device management, compatible with domestic GPUs
 */

#include "GPUCore.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

#ifdef ENABLE_OPENCL

namespace cnlab {
namespace gpu {

// Domestic GPU vendors list
static const char* DOMESTIC_VENDORS[] = {
    "Moore Threads",
    "Jingjia",
    "Hygon",
    "Innosilicon",
    "Zhaoxin"
};

static bool isDomesticVendor(const std::string& vendor) {
    std::string vendorLower = vendor;
    std::transform(vendorLower.begin(), vendorLower.end(), vendorLower.begin(), ::tolower);
    
    for (const auto& domestic : DOMESTIC_VENDORS) {
        std::string domesticLower = domestic;
        std::transform(domesticLower.begin(), domesticLower.end(), domesticLower.begin(), ::tolower);
        if (vendorLower.find(domesticLower) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void GPUCore::checkError(cl_int err, const std::string& operation) {
    if (err != CL_SUCCESS) {
        throw GPUException(operation + " failed with error code: " + std::to_string(err));
    }
}

GPUCore& GPUCore::getInstance() {
    static GPUCore instance;
    return instance;
}

bool GPUCore::initialize() {
    if (initialized_) {
        return true;
    }

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    
    if (platforms.empty()) {
        throw GPUException("No OpenCL platforms found");
    }
    
    platform_ = platforms[0];
    
    std::vector<cl::Device> allDevices;
    cl_int err = platform_.getDevices(CL_DEVICE_TYPE_GPU, &allDevices);
    
    if (err != CL_SUCCESS || allDevices.empty()) {
        throw GPUException("No GPU devices found on platform");
    }
    
    devices_ = allDevices;
    
    device_ = devices_[0];
    
    context_ = cl::Context(device_);
    queue_ = cl::CommandQueue(context_, device_);
    
    initialized_ = true;
    return true;
}

void GPUCore::shutdown() {
    if (!initialized_) {
        return;
    }
    
    try {
        queue_.finish();
    } catch (...) {}
    
    queue_ = cl::CommandQueue();
    context_ = cl::Context();
    device_ = cl::Device();
    devices_.clear();
    
    initialized_ = false;
}

std::vector<GPUDeviceInfo> GPUCore::enumerateDevices() {
    std::vector<GPUDeviceInfo> infos;
    
    if (!initialized_) {
        throw GPUException("GPU not initialized");
    }
    
    for (size_t i = 0; i < devices_.size(); ++i) {
        GPUDeviceInfo info;
        info.name = devices_[i].getInfo<CL_DEVICE_NAME>();
        info.vendor = devices_[i].getInfo<CL_DEVICE_VENDOR>();
        info.version = devices_[i].getInfo<CL_DEVICE_VERSION>();
        info.globalMemory = devices_[i].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
        info.localMemory = devices_[i].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
        info.maxWorkGroupSize = devices_[i].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
        info.maxWorkItemSizes = devices_[i].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
        info.computeUnits = devices_[i].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
        info.isAvailable = devices_[i].getInfo<CL_DEVICE_AVAILABLE>();
        infos.push_back(info);
    }
    
    return infos;
}

bool GPUCore::selectDevice(size_t index) {
    if (!initialized_) {
        throw GPUException("GPU not initialized");
    }
    
    if (index >= devices_.size()) {
        throw GPUException("Invalid device index: " + std::to_string(index));
    }
    
    queue_.finish();
    
    device_ = devices_[index];
    context_ = cl::Context(device_);
    queue_ = cl::CommandQueue(context_, device_);
    
    return true;
}

bool GPUCore::selectDeviceByName(const std::string& name) {
    if (!initialized_) {
        throw GPUException("GPU not initialized");
    }
    
    for (size_t i = 0; i < devices_.size(); ++i) {
        std::string deviceName = devices_[i].getInfo<CL_DEVICE_NAME>();
        if (deviceName.find(name) != std::string::npos) {
            return selectDevice(i);
        }
    }
    
    throw GPUException("Device not found: " + name);
}

bool GPUCore::selectDomesticGPU() {
    if (!initialized_) {
        throw GPUException("GPU not initialized");
    }
    
    for (size_t i = 0; i < devices_.size(); ++i) {
        std::string vendor = devices_[i].getInfo<CL_DEVICE_VENDOR>();
        if (isDomesticVendor(vendor)) {
            return selectDevice(i);
        }
    }
    
    return false;
}

GPUDeviceInfo GPUCore::getCurrentDeviceInfo() const {
    if (!initialized_) {
        throw GPUException("GPU not initialized");
    }
    
    GPUDeviceInfo info;
    info.name = device_.getInfo<CL_DEVICE_NAME>();
    info.vendor = device_.getInfo<CL_DEVICE_VENDOR>();
    info.version = device_.getInfo<CL_DEVICE_VERSION>();
    info.globalMemory = device_.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
    info.localMemory = device_.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
    info.maxWorkGroupSize = device_.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    info.maxWorkItemSizes = device_.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    info.computeUnits = device_.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
    info.isAvailable = device_.getInfo<CL_DEVICE_AVAILABLE>();
    
    return info;
}

std::string GPUCore::getGPUInfo() const {
    if (!initialized_) {
        return "GPU not initialized";
    }
    
    GPUDeviceInfo info = getCurrentDeviceInfo();
    
    std::string result = "GPU Device Information:\n";
    result += "  Name: " + info.name + "\n";
    result += "  Vendor: " + info.vendor;
    if (info.isDomesticGPU()) {
        result += " (Domestic GPU)";
    }
    result += "\n";
    result += "  Version: " + info.version + "\n";
    result += "  Global Memory: " + std::to_string(info.globalMemory / 1024 / 1024) + " MB\n";
    result += "  Local Memory: " + std::to_string(info.localMemory / 1024) + " KB\n";
    result += "  Compute Units: " + std::to_string(info.computeUnits) + "\n";
    result += "  Max Work Group Size: " + std::to_string(info.maxWorkGroupSize) + "\n";
    
    return result;
}

bool GPUDeviceInfo::isDomesticGPU() const {
    return isDomesticVendor(vendor);
}

bool initializeGPU() {
    return GPUCore::getInstance().initialize();
}

void shutdownGPU() {
    GPUCore::getInstance().shutdown();
}

bool isGPUAvailable() {
    return GPUCore::getInstance().isInitialized();
}

std::string getGPUInfo() {
    return GPUCore::getInstance().getGPUInfo();
}

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
