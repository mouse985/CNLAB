#include "MemoryPool.hpp"
#include <cstdlib>
#include <algorithm>

namespace cnlab {

MemoryPool& MemoryPool::getInstance() {
    static MemoryPool instance;
    return instance;
}

MemoryPool::MemoryPool() {
    pools_.reserve(16);
}

MemoryPool::~MemoryPool() {
    clear();
}

void* MemoryPool::allocate(size_t size) {
    if (size == 0) return nullptr;
    
    if (size > MAX_POOL_SIZE) {
        return std::malloc(size);
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    Pool* pool = findPool(size);
    if (!pool) {
        Pool newPool;
        newPool.blockSize = (size + 7) & ~7;
        newPool.blockCount = 64;
        pools_.push_back(std::move(newPool));
        pool = &pools_.back();
        expandPool(*pool);
    }
    
    if (pool->freeBlocks.empty()) {
        expandPool(*pool);
    }
    
    void* ptr = pool->freeBlocks.back();
    pool->freeBlocks.pop_back();
    return ptr;
}

void MemoryPool::deallocate(void* ptr, size_t size) {
    if (!ptr) return;
    
    if (size > MAX_POOL_SIZE) {
        std::free(ptr);
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    Pool* pool = findPool(size);
    if (pool) {
        pool->freeBlocks.push_back(ptr);
    } else {
        std::free(ptr);
    }
}

void* MemoryPool::alignedAllocate(size_t size, size_t alignment) {
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    posix_memalign(&ptr, alignment, size);
    return ptr;
#endif
}

void MemoryPool::alignedDeallocate(void* ptr) {
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

void MemoryPool::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pool : pools_) {
        for (auto& chunk : pool.chunks) {
            chunk.reset();
        }
        pool.freeBlocks.clear();
    }
    pools_.clear();
}

MemoryPool::Pool* MemoryPool::findPool(size_t size) {
    size_t alignedSize = (size + 7) & ~7;
    
    for (auto& pool : pools_) {
        if (pool.blockSize == alignedSize) {
            return &pool;
        }
    }
    return nullptr;
}

void MemoryPool::expandPool(Pool& pool) {
    size_t chunkSize = pool.blockSize * pool.blockCount;
    auto chunk = std::make_unique<char[]>(chunkSize);
    
    for (size_t i = 0; i < pool.blockCount; ++i) {
        pool.freeBlocks.push_back(chunk.get() + i * pool.blockSize);
    }
    
    pool.chunks.push_back(std::move(chunk));
    pool.blockCount *= 2;
}

}
