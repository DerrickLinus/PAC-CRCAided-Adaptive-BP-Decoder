cmake_minimum_required(VERSION 3.15)
project(ABPDecoder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release 优化标志
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -fopenmp")

# 主程序源文件
set(SOURCES
    main.cpp
    Channel.cpp
    CRC.cpp
    CTool.cpp
    Decode.cpp
    Initial.cpp
    Simulation.cpp
)

add_executable(ABPDecoder ${SOURCES})
