cmake_minimum_required (VERSION 2.8.11)
project(unjit)

find_package(LLVM ${LLVM_VERSION} REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})

add_executable(unjit src/unjit.cpp src/Process.cpp src/Disassembler.cpp)
add_definitions(-D_XOPEN_SOURCE=700)

# llvm_map_components_to_libnames(llvm_libs support core mcdisassembler native)
# target_link_libraries(unjit ${llvm_libs})

target_link_libraries(unjit LLVM-${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR})
set_property(TARGET unjit PROPERTY CXX_STANDARD 11)
set_property(TARGET unjit PROPERTY CXX_STANDARD_REQUIRED ON)
