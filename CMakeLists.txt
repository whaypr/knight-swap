cmake_minimum_required(VERSION 3.28)
project(knight_swap)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenMP REQUIRED)
add_executable(knight_swap src/main.cpp
        src/InputData.h
        src/Types.h
        src/Solver.h
        src/InstanceInfo.h
        src/BoardState.h
)
target_link_libraries(knight_swap PRIVATE OpenMP::OpenMP_CXX)