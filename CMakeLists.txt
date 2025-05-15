cmake_minimum_required(VERSION 3.16)
project(CustomMemoryPool VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 라이브러리 타겟 설정
add_library(CustomMemoryPool
    src/MemoryCommon.cpp
    src/MemoryPool.cpp
)

target_sources(CustomMemoryPool 
PUBLIC 
    include/MemoryCommon.h
    include/MemoryPool.h
    
)

target_include_directories(CustomMemoryPool
    PUBLIC include
)


include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(MemoryPoolTests 
    tests/MemoryPoolTests.cpp
    tests/MemoryManagerTests.cpp
)
target_link_libraries(MemoryPoolTests
    CustomMemoryPool
    gtest_main
)

add_test(NAME MemoryPoolTests COMMAND MemoryPoolTests)