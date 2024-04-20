cmake_minimum_required(VERSION 3.28)
project(knight_swap)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)

add_executable(knight_swap src/main.cpp
        src/InputData.h
        src/Types.h
        src/SolverMaster.h
        src/InstanceInfo.h
        src/BoardState.h
        src/SolverSlave.h
        src/InstanceInfoBuilder.h
        src/BoardStateBuilder.h
)

target_link_libraries(knight_swap PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(knight_swap PRIVATE mpi)
