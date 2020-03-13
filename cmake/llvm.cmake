#
# MIT License
# Copyright (c) 2018 James, https://github.com/zhuguangxiang
#

FIND_PACKAGE(LLVM REQUIRED CONFIG)

MESSAGE(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
MESSAGE(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
MESSAGE(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
MESSAGE(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
llvm_map_components_to_libnames(llvm_libs core executionengine interpreter native mcjit)
MESSAGE(STATUS "LLVM_DEFINITIONS: ${llvm_libs}")