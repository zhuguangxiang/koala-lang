#
# This file is part of the koala project with MIT License.
# Copyright (c) zhuguangxiang <zhuguangxiang@gmail.com>.
#

# Install the std library tree (dist directory) verbatim under lib/, so the
# standard library sits next to libkoala.so, e.g. ~/.local/lib/koala-0.7/lib/std/.
# Third-party packages (koala/* and others) go to lib/koala-packages/ instead,
# installed by koala-libs' own install.sh.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/libs/std/dist/
    DESTINATION koala-${KOALA_VERSION_MAIN}/lib)

include(${PROJECT_SOURCE_DIR}/cmake/symlink.cmake)
