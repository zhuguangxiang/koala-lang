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

include(${PROJECT_SOURCE_DIR}/cmake/symlink.cmake)

install_symlink(
    ./koala/lib/libkoala_ext_native.so
    ${CMAKE_INSTALL_PREFIX}/../lib/libkoala_ext_native.so)

# Package-specific compiler arguments, in dependency order.
set(BUILTIN_ARGS --build-stdlib std/builtin --package-name=std/builtin)
set(IO_ARGS --tail-call std/io --package-name=std/io)
set(FS_ARGS --tail-call std/fs --package-name=std/fs)
set(OS_ARGS --tail-call std/os --package-name=std/os)
set(SYS_ARGS --tail-call std/sys --package-name=std/sys)
set(PRELUDE_ARGS --tail-call std/prelude --package-name=std/prelude)
set(TIME_ARGS --tail-call std/time --package-name=std/time)
set(UT_ARGS --tail-call std/ut --package-name=std/ut)
set(PRETTY_ARGS --tail-call koala/pretty.kl --package-name=koala/pretty)

set(KOALA_LIBS BUILTIN IO FS OS SYS PRELUDE TIME UT PRETTY)
set(KOALA_LIB_COMMANDS)
foreach(lib IN LISTS KOALA_LIBS)
    list(APPEND KOALA_LIB_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E env "KOALA_PATH=${CMAKE_CURRENT_SOURCE_DIR}/"
            "$<TARGET_FILE:koalac_out>" --cgen --write-klc --fusion ${${lib}_ARGS}
    )
endforeach()

add_custom_target(koala_std_lib ALL
    ${KOALA_LIB_COMMANDS}
    DEPENDS koalac_out
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Compiling Koala libraries"
    VERBATIM
)
