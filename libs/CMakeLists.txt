#
# This file is part of the koala project with MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
#

# Install the std/koala library trees (dist directories) verbatim
# under koala-lib/, e.g. ~/.local/lib/koala-0.7/lib/koala-lib/{std,koala}/.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/libs/std/dist/
    DESTINATION koala-${KOALA_VERSION_MAIN}/lib/koala-lib)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/libs/koala/dist/
    DESTINATION koala-${KOALA_VERSION_MAIN}/lib/koala-lib)

include(${PROJECT_SOURCE_DIR}/cmake/symlink.cmake)
