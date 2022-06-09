#
# This file is part of the koala-lang project, under the MIT License.
# Copyright (c) 2018-2022 James <zhuguangxiang@gmail.com>
#

FIND_PACKAGE(LLVM REQUIRED CONFIG)

MESSAGE(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
MESSAGE(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
#add_definitions(${LLVM_DEFINITIONS})
MESSAGE(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
MESSAGE(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
llvm_map_components_to_libnames(llvm_libs core executionengine interpreter native mcjit)
MESSAGE(STATUS "LLVM_LIBS: ${llvm_libs}")
