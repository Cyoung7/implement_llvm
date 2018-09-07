cmake_minimum_required(VERSION 3.10)
project(implement_llvm)

#set(LLVM_DIR /media/cyoung/000E88CC0009670E/llvm/build_install/lib/cmake/llvm)
set(LLVM_DIR /home/cyoung/llvm/build_install/lib/cmake/llvm)


find_package(LLVM REQUIRED CONFIG )


message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_executable(implement_llvm main.cpp)