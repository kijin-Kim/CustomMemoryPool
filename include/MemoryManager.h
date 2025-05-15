#pragma once

#include "MemoryPool.h"

namespace Memory
{
    class MemoryManager
    {
    public:
        static MemoryManager& GetInstance();
        
        ~MemoryManager();
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&) = delete;
        MemoryManager& operator=(MemoryManager&&) = delete;
        
        void Initialize();
        void Shutdown() const;

        void* Allocate(size_t size) const;
        void Deallocate(void* ptr) const;

    private:
        MemoryManager() = default;
        MemoryPool* selectPoolForSize(size_t size) const;

    private:
        MemoryPool* mpSmallBlockPool = nullptr; // 64B 블록
        MemoryPool* mpMediumBlockPool = nullptr; // 256B 블록
        MemoryPool* mpLargeBlockPool = nullptr; // 1KB 블록
        bool mbInitialized = false;
    };
}
