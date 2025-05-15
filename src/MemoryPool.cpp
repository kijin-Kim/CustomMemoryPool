#include "MemoryPool.h"

#include <algorithm>
#include "MemoryCommon.h"

namespace Memory
{
    MemoryPool::MemoryPool(size_t totalSize, size_t blockSize)
        : mpPoolMemory(nullptr),
          mTotalSize(totalSize),
          mBlockSize(blockSize),
          mBlockCount(0),
          mUsedBlockCount(0),
          mpFreeList(nullptr),
          mpBitmap(nullptr),
          mbInitialized(false)
    {
        // 블록 크기는 최소한 헤더 크기 + 포인터 크기보다 커야 함
        Assert(mBlockSize >= sizeof(BlockHeader) + sizeof(void*), "블록 크기가 너무 작습니다.");
        Assert((mBlockSize % 8) == 0, "블록 크기는 8의 배수여야 합니다.");
        Assert((mTotalSize % mBlockSize) == 0, "총 메모리 크기는 블록 크기의 배수여야 합니다.");

        mBlockCount = mTotalSize / mBlockSize;
        Assert(mBlockCount > 0, "메모리 풀에 최소 1개 이상의 블록이 필요합니다.");

        initialize();
    }

    MemoryPool::~MemoryPool()
    {
#ifdef _DEBUG
        if (mUsedBlockCount > 0)
        {
            Assert(false, "메모리 누수 감지: 사용중인 블록이 있습니다.");
        }
#endif

        delete[] mpPoolMemory;
        delete[] mpBitmap;
    }
    
    void* MemoryPool::Allocate(size_t size)
    {
        Assert(mbInitialized, "메모리 풀이 초기화되지 않았습니다");
        Assert(size <= mBlockSize - sizeof(BlockHeader), "요청 크기가 너무 큽니다");

        void* blockPtr = findFreeBlock(size);
        if (!blockPtr)
        {
            return nullptr;
        }


        BlockHeader* const header = static_cast<BlockHeader*>(blockPtr);
        header->AllocationFlag = 1;


        const size_t blockIndex = (static_cast<uint8_t*>(blockPtr) - mpPoolMemory) / mBlockSize;
        mpBitmap[blockIndex / 64] |= 1ULL << (blockIndex % 64);

        ++mUsedBlockCount;

        return static_cast<uint8_t*>(blockPtr) + sizeof(BlockHeader);
    }

    void MemoryPool::Deallocate(void* ptr)
    {
        if (!ptr)
        {
            return;
        }

        Assert(mbInitialized, "메모리 풀이 초기화되지 않았습니다");

        // 사용자 포인터에서 블록 시작 주소 계산 (헤더 위치)
        void* const blockPtr = static_cast<uint8_t*>(ptr) - sizeof(BlockHeader);

        Assert(IsValidPoolPointer(blockPtr), "잘못된 포인터가 Deallocate에 전달되었습니다");

        BlockHeader* const header = static_cast<BlockHeader*>(blockPtr);
        Assert(header->AllocationFlag == 1, "이미 해제된 블록을 Deallocate하려고 합니다");
        header->AllocationFlag = 0;

        const size_t blockIndex = (static_cast<uint8_t*>(blockPtr) - mpPoolMemory) / mBlockSize;
        mpBitmap[blockIndex / 64] &= ~(1ULL << (blockIndex % 64));

        addToFreeList(blockPtr);

        --mUsedBlockCount;
    }

    size_t MemoryPool::GetUsedBlockCount() const
    {
        return mUsedBlockCount;
    }

    double MemoryPool::GetFragmentationRatio() const
    {
        if (mUsedBlockCount <= 1 || mUsedBlockCount >= mBlockCount)
        {
            return 0.0; // 사용 블록이 없거나 1개뿐이거나 모든 블록이 사용 중이면 단편화 없음
        }

        // 할당-비할당 간 전환 횟수
        size_t transitions = 0;
        bool bPrevAllocated = false;

        for (size_t i = 0; i < mBlockCount; ++i)
        {
            const size_t wordIndex = i / 64;
            const uint64_t bitMask = 1ULL << (i % 64);
            const bool bIsAllocated = (mpBitmap[wordIndex] & bitMask) != 0;

            if (i > 0 && bIsAllocated != bPrevAllocated)
            {
                ++transitions;
            }

            bPrevAllocated = bIsAllocated;
        }

        // 단편화 비율: 전환 횟수 / (2 * 할당된 블록 수)
        // 완전 단편화: 모든 할당 블록이 분산됨 = 1.0
        // 최소 단편화: 모든 할당 블록이 연속됨 = 0.0
        return std::min(1.0, static_cast<double>(transitions) / (2.0 * static_cast<double>(mUsedBlockCount)));
    }

    bool MemoryPool::IsValidPoolPointer(void* ptr) const
    {
        // 포인터가 메모리 풀 범위 내에 있는지 확인
        if (ptr < mpPoolMemory || ptr >= mpPoolMemory + mTotalSize)
        {
            return false;
        }

        // 포인터가 블록 경계에 맞게 정렬되어 있는지 확인
        if ((static_cast<uint8_t*>(ptr) - mpPoolMemory) % mBlockSize != 0)
        {
            return false;
        }

        const BlockHeader* const header = static_cast<BlockHeader*>(ptr);

        if (header->MagicNumber != MAGIC_NUMBER)
        {
            return false;
        }

        if (header->PoolID != static_cast<uint16_t>(reinterpret_cast<uintptr_t>(this) & 0xFFFF))
        {
            return false;
        }

        return true;
    }

    void MemoryPool::initialize()
    {
        mpPoolMemory = new uint8_t[mTotalSize];
        Assert(mpPoolMemory, "메모리 풀 할당 실패");

        // 비트맵 초기화 (각 블록당 1비트)
        const size_t bitmapSizeInBytes = (mBlockCount + 63) / 64 * sizeof(uint64_t);
        mpBitmap = new uint64_t[bitmapSizeInBytes / sizeof(uint64_t)];
        Assert(mpBitmap, "비트맵 할당 실패");
        memset(mpBitmap, 0, bitmapSizeInBytes);


        mpFreeList = nullptr;

        // 뒤에서부터 프리 리스트 구성 (LIFO 순서)
        for (int i = static_cast<int>(mBlockCount) - 1; i >= 0; --i)
        {
            void* const blockPtr = mpPoolMemory + i * mBlockSize;

            BlockHeader* const header = static_cast<BlockHeader*>(blockPtr);
            header->MagicNumber = MAGIC_NUMBER;
            header->PoolID = static_cast<uint16_t>(reinterpret_cast<uintptr_t>(this) & 0xFFFF);
            header->AllocationFlag = 0;
            header->Padding = 0;

            addToFreeList(blockPtr);
        }

        mbInitialized = true;
    }

    void MemoryPool::addToFreeList(void* ptr)
    {
        // 다음 블록의 넥스트 포인터를 가르키는 포인터
        void** const nextPtr = reinterpret_cast<void**>(static_cast<uint8_t*>(ptr) + sizeof(BlockHeader));
        // 현재 블록의 주소를 이전 블록의 넥스트 포인터에 저장
        *nextPtr = mpFreeList;
        mpFreeList = ptr;
    }

    void* MemoryPool::findFreeBlock(size_t size)
    {
        Assert(mbInitialized, "메모리 풀이 초기화되지 않았습니다.");
        Assert(size <= mBlockSize - sizeof(BlockHeader), "요청 크기가 블록 크기를 초과합니다.");

        if (!mpFreeList)
        {
            return nullptr;
        }

        void* const block = mpFreeList;
        // 현재 프리 블록의 넥스트 포인터를 가르키는 포인터
        void* const* const nextPtr = reinterpret_cast<void**>(static_cast<uint8_t*>(block) + sizeof(BlockHeader));
        mpFreeList = *nextPtr;

        return block;
    }
}
