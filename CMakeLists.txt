cmake_minimum_required(VERSION 3.5)
project(implement_llvm)
set(CMAKE_CXX_STANDARD 11)

set(LLVM_DIR /media/cyoung/000E88CC0009670E/llvm/build_install/lib/cmake/llvm)
#set(LLVM_DIR /home/cyoung/llvm/build_install/lib/cmake/llvm)

#添加cmake指令
set (CMAKE_MODULE_PATH ${LLVM_DIR})
include(${LLVM_DIR}/AddLLVM.cmake)




find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})



add_subdirectory(lib)


# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
#target_link_libraries(implement_llvm ${llvm_libs})
