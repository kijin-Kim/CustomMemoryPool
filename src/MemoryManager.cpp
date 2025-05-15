#include "MemoryManager.h"

#include "MemoryCommon.h"

namespace Memory
{
    MemoryManager& MemoryManager::GetInstance()
    {
        static MemoryManager instance;
        return instance;
    }

    MemoryManager::~MemoryManager()
    {
        Shutdown();
    }

    void MemoryManager::Initialize()
    {
        if (!mbInitialized)
        {
            mpSmallBlockPool = new MemoryPool(2LL * 1024 * 1024, SMALL_BLOCK_SIZE); // 2MB
            mpMediumBlockPool = new MemoryPool(1LL * 1024 * 1024, MEDIUM_BLOCK_SIZE); // 1MB
            mpLargeBlockPool = new MemoryPool(1LL * 1024 * 1024, LARGE_BLOCK_SIZE); // 1MB

            mbInitialized = true;
        }
    }

    void MemoryManager::Shutdown() const
    {
        if (mbInitialized)
        {
            delete mpSmallBlockPool;
            delete mpMediumBlockPool;
            delete mpLargeBlockPool;
        }
    }

    void* MemoryManager::Allocate(size_t size) const
    {
        if (!mbInitialized)
        {
            return new uint8_t[size];
        }

        MemoryPool* const pool = selectPoolForSize(size);

        if (!pool)
        {
            return new uint8_t[size];
        }

        void* ptr = pool->Allocate(size);

        if (!ptr)
        {
            return new uint8_t[size];
        }

        return ptr;
    }

    void MemoryManager::Deallocate(void* ptr) const
    {
        if (!ptr)
        {
            return;
        }

        if (!mbInitialized)
        {
            delete[] static_cast<uint8_t*>(ptr);
            return;
        }

        uint8_t* const blockPtr = static_cast<uint8_t*>(ptr) - sizeof(BlockHeader);
        
        if (mpSmallBlockPool && mpSmallBlockPool->IsValidPoolPointer(blockPtr))
        {
            mpSmallBlockPool->Deallocate(ptr);
            return;
        }
        
        if (mpMediumBlockPool && mpMediumBlockPool->IsValidPoolPointer(blockPtr))
        {
            mpMediumBlockPool->Deallocate(ptr);
            return;
        }
        
        if (mpLargeBlockPool && mpLargeBlockPool->IsValidPoolPointer(blockPtr))
        {
            mpLargeBlockPool->Deallocate(ptr);
            return;
        }

        delete[] static_cast<uint8_t*>(ptr);
    }

    MemoryPool* MemoryManager::selectPoolForSize(size_t size) const
    {
        const size_t requiredSize = size + sizeof(BlockHeader);
        
        if (requiredSize <= SMALL_BLOCK_SIZE)
        {
            return mpSmallBlockPool;
        }
        
        if (requiredSize <= MEDIUM_BLOCK_SIZE)
        {
            return mpMediumBlockPool;
        }
        
        if (requiredSize <= LARGE_BLOCK_SIZE)
        {
            return mpLargeBlockPool;
        }
        
        return nullptr;
    }

}
