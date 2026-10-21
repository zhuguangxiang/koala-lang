#
# This file is part of the koala project with MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
#

set(KOALA_EXT_NATIVE_SOURCES
    koala/native/pretty.c
    )

add_library(koala_ext_native SHARED ${KOALA_EXT_NATIVE_SOURCES})
target_link_libraries(koala_ext_native koala)

install(TARGETS koala_ext_native DESTINATION koala-${KOALA_VERSION_MAIN}/lib)

# Install the std/koala library trees (dist directories) verbatim
# under koala-lib/, e.g. ~/.local/lib/koala-0.7/lib/koala-lib/{std,koala}/.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/libs/std/dist/
    DESTINATION koala-${KOALA_VERSION_MAIN}/lib/koala-lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/libs/koala/dist/
    DESTINATION koala-${KOALA_VERSION_MAIN}/lib/koala-lib)

include(${PROJECT_SOURCE_DIR}/cmake/symlink.cmake)

install_symlink(
    ./koala/lib/libkoala_ext_native.so
    ${CMAKE_INSTALL_PREFIX}/../lib/libkoala_ext_native.so)
