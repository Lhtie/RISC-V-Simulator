cmake_minimum_required(VERSION 3.16)
project(RISC_V_Simulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(RISC_V_Simulator main.cpp hex.hpp decode.hpp memory.hpp)