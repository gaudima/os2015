cmake_minimum_required(VERSION 3.5)
project(rshd)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES rshd.cpp)
add_executable(rshd ${SOURCE_FILES})