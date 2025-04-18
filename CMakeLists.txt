cmake_minimum_required(VERSION 3.30)
project(CPPTest)

set(CMAKE_CXX_STANDARD 20)

add_executable(CPPTest src/main.cpp
        src/writebuffer.cpp
        src/writebuffer.hpp
        src/readbuffer.cpp
        src/readbuffer.hpp
        src/uuid.cpp
        src/uuid.hpp)

target_link_libraries(CPPTest)
