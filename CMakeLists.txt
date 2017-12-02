cmake_minimum_required(VERSION 3.8)
project(RateMonotonicScheduler)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
add_executable(RateMonotonicScheduler ${SOURCE_FILES})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
target_link_libraries(RateMonotonicScheduler -lrt)