#pragma once

#include <cassert>
#include <cstdint>

namespace Memory
{
#ifdef _DEBUG
#define Assert(expression, message) if(!(expression)) assert((expression) && (message))
#else
    #define Assert(expression, message) if(!(expression)) __assume(0)
#endif

    // 메모리 블록 헤더 구조체
    struct BlockHeader
    {
        uint32_t MagicNumber;
        uint16_t PoolID;
        uint8_t AllocationFlag; // 할당 여부 (0=빈 블록, 1=할당)
        uint8_t Padding;
    };

    static_assert(sizeof(BlockHeader) == 8);

    constexpr size_t CACHE_LINE_SIZE = 64;

    // 블록 크기
    constexpr size_t SMALL_BLOCK_SIZE = 64;
    constexpr size_t MEDIUM_BLOCK_SIZE = 256;
    constexpr size_t LARGE_BLOCK_SIZE = 1024;

    // 블록 유효성 검증을 위한 매직 넘버
    constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;
}
