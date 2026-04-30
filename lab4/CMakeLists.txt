cmake_minimum_required(VERSION 3.10.0)
project(lab4)

set(prog_name GameOfLive)

find_package(MPI REQUIRED)

add_executable(${prog_name} main.cpp BitArray.cpp  Cell.cpp GameField.cpp Rule.cpp GameOfLive.cpp Globals.h)

target_compile_options(${prog_name} PRIVATE -Ofast)

target_link_libraries(${prog_name} PRIVATE MPI::MPI_CXX)
