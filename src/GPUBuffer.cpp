/*
 * MatlabCN GPU Buffer Management Implementation (Domestic Implementation)
 * OpenCL memory management, compatible with domestic GPUs
 */

#include "GPUBuffer.hpp"
#include "GPUCore.hpp"

#ifdef ENABLE_OPENCL

namespace cnlab {
namespace gpu {

// Explicit instantiations for common types
template class GPUBuffer<float>;
template class GPUBuffer<double>;

template<typename T>
GPUBuffer<T>::GPUBuffer() = default;

template<typename T>
GPUBuffer<T>::GPUBuffer(size_t size, cl_mem_flags flags) {
    allocate(size, flags);
}

template<typename T>
GPUBuffer<T>::~GPUBuffer() {
    release();
}

template<typename T>
GPUBuffer<T>::GPUBuffer(GPUBuffer&& other) noexcept
    : buffer_(std::move(other.buffer_))
    , size_(other.size_)
    , flags_(other.flags_) {
    other.size_ = 0;
}

template<typename T>
GPUBuffer<T>& GPUBuffer<T>::operator=(GPUBuffer&& other) noexcept {
    if (this != &other) {
        release();
        buffer_ = std::move(other.buffer_);
        size_ = other.size_;
        flags_ = other.flags_;
        other.size_ = 0;
    }
    return *this;
}

template<typename T>
void GPUBuffer<T>::allocate(size_t size, cl_mem_flags flags) {
    if (!GPUCore::getInstance().isInitialized()) {
        throw GPUException("GPU not initialized");
    }
    
    release();
    
    cl::Context& context = GPUCore::getInstance().getContext();
    buffer_ = cl::Buffer(context, flags, size * sizeof(T));
    size_ = size;
    flags_ = flags;
}

template<typename T>
void GPUBuffer<T>::release() {
    if (size_ > 0) {
        buffer_ = cl::Buffer();
        size_ = 0;
    }
}

template<typename T>
void GPUBuffer<T>::upload(const T* hostData, size_t size, bool blocking) {
    if (!isValid()) {
        throw GPUException("GPU buffer not allocated");
    }
    
    if (size > size_) {
        throw GPUException("Upload size exceeds buffer size");
    }
    
    cl::CommandQueue& queue = GPUCore::getInstance().getQueue();
    cl_int err = queue.enqueueWriteBuffer(
        buffer_,
        blocking ? CL_TRUE : CL_FALSE,
        0,
        size * sizeof(T),
        hostData
    );
    GPUCore::checkError(err, "Buffer upload");
    
    if (blocking) {
        queue.finish();
    }
}

template<typename T>
void GPUBuffer<T>::download(T* hostData, size_t size, bool blocking) const {
    if (!isValid()) {
        throw GPUException("GPU buffer not allocated");
    }
    
    if (size > size_) {
        throw GPUException("Download size exceeds buffer size");
    }
    
    cl::CommandQueue& queue = GPUCore::getInstance().getQueue();
    cl_int err = queue.enqueueReadBuffer(
        buffer_,
        blocking ? CL_TRUE : CL_FALSE,
        0,
        size * sizeof(T),
        hostData
    );
    GPUCore::checkError(err, "Buffer download");
    
    if (blocking) {
        queue.finish();
    }
}

template<typename T>
T* GPUBuffer<T>::map(cl_map_flags flags, size_t size) {
    if (!isValid()) {
        throw GPUException("GPU buffer not allocated");
    }
    
    if (size > size_) {
        throw GPUException("Map size exceeds buffer size");
    }
    
    cl::CommandQueue& queue = GPUCore::getInstance().getQueue();
    cl_int err;
    T* ptr = static_cast<T*>(queue.enqueueMapBuffer(
        buffer_,
        CL_TRUE,
        flags,
        0,
        size * sizeof(T),
        nullptr,
        nullptr,
        &err
    ));
    GPUCore::checkError(err, "Buffer map");
    return ptr;
}

template<typename T>
void GPUBuffer<T>::unmap(T* mappedPtr) {
    if (!isValid() || !mappedPtr) {
        throw GPUException("Invalid buffer or mapped pointer");
    }
    
    cl::CommandQueue& queue = GPUCore::getInstance().getQueue();
    cl_int err = queue.enqueueUnmapMemObject(buffer_, mappedPtr);
    GPUCore::checkError(err, "Buffer unmap");
    queue.finish();
}

template<typename T>
void GPUBuffer<T>::fill(const T& pattern, size_t size) {
    if (!isValid()) {
        throw GPUException("GPU buffer not allocated");
    }
    
    if (size > size_) {
        throw GPUException("Fill size exceeds buffer size");
    }
    
    cl::CommandQueue& queue = GPUCore::getInstance().getQueue();
    cl_int err = queue.enqueueFillBuffer(
        buffer_,
        pattern,
        0,
        size * sizeof(T)
    );
    GPUCore::checkError(err, "Buffer fill");
    queue.finish();
}

} // namespace gpu
} // namespace cnlab

#endif // ENABLE_OPENCL
