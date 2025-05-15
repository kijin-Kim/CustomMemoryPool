#pragma once

#include <cstdint>

namespace Memory
{
    class MemoryPool
    {
    public:
        MemoryPool(size_t totalSize, size_t blockSize);
        ~MemoryPool();
        MemoryPool(const MemoryPool&) = delete;
        MemoryPool& operator=(const MemoryPool&) = delete;
        MemoryPool(MemoryPool&&) = delete;
        MemoryPool& operator=(MemoryPool&&) = delete;
        
        void* Allocate(size_t size);
        void Deallocate(void* ptr);
        
        size_t GetUsedBlockCount() const;
        double GetFragmentationRatio() const;
        bool IsValidPoolPointer(void* ptr) const;
        
    private:
        void initialize();
        void* findFreeBlock(size_t size);
        void addToFreeList(void* ptr);
        

    private:
        // 전체 메모리 풀 포인터
        uint8_t* mpPoolMemory;
        // 총 메모리 크기
        size_t mTotalSize;
        // 블록 크기
        size_t mBlockSize;
        // 총 블록 수
        size_t mBlockCount;
        // 사용중인 블록 수
        size_t mUsedBlockCount;
        // 빈 블록 리스트
        void* mpFreeList;
        // 할당 비트맵
        uint64_t* mpBitmap;
        // 초기화 여부
        bool mbInitialized;
    };
}
