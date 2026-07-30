#
# This file is part of the koala project with MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
#

set(KOALA_NATIVE_SOURCES
    std/native/fs.c
    std/native/io.c
    std/native/os.c
    std/native/time.c
    )

add_library(koala_std_native SHARED ${KOALA_NATIVE_SOURCES})
target_link_libraries(koala_std_native koala)

set(KOALA_EXT_NATIVE_SOURCES
    koala/native/pretty.c
    )

add_library(koala_ext_native SHARED ${KOALA_EXT_NATIVE_SOURCES})
target_link_libraries(koala_ext_native koala)

install(TARGETS koala_std_native DESTINATION koala-${KOALA_VERSION_MAIN}/lib)
install(TARGETS koala_ext_native DESTINATION koala-${KOALA_VERSION_MAIN}/lib)

include(${PROJECT_SOURCE_DIR}/cmake/symlink.cmake)

install_symlink(
    ./koala/lib/libkoala_std_native.so
    ${CMAKE_INSTALL_PREFIX}/../lib/libkoala_std_native.so)

install_symlink(
    ./koala/lib/libkoala_ext_native.so
    ${CMAKE_INSTALL_PREFIX}/../lib/libkoala_ext_native.so)

# Compile koala std libraries
# add_custom_target(koala_std_lib ALL
#     COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/build.sh
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#     COMMENT "Building koala std libraries"
#     VERBATIM
# )
