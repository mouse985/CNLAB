#pragma once

#include <cstddef>
#include <vector>
#include <memory>
#include <mutex>

namespace cnlab {

class MemoryPool {
public:
    static MemoryPool& getInstance();
    
    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
    
    void* alignedAllocate(size_t size, size_t alignment);
    void alignedDeallocate(void* ptr);
    
    void clear();
    
private:
    MemoryPool();
    ~MemoryPool();
    
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    
    struct Pool {
        size_t blockSize;
        size_t blockCount;
        std::vector<void*> freeBlocks;
        std::vector<std::unique_ptr<char[]>> chunks;
    };
    
    std::vector<Pool> pools_;
    std::mutex mutex_;
    
    static constexpr size_t MAX_POOL_SIZE = 1024 * 1024;
    static constexpr size_t ALIGNMENT = 64;
    
    Pool* findPool(size_t size);
    void expandPool(Pool& pool);
};

}
