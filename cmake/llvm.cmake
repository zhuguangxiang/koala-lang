#
# This file is part of the koala project with MIT LIcense.
# Copyright (c) James <zhuguangxiang@gmail.com>
#

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
#add_definitions(${LLVM_DEFINITIONS})
message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
llvm_map_components_to_libnames(llvm_libs core executionengine interpreter native mcjit)
message(STATUS "LLVM_LIBS: ${llvm_libs}")
