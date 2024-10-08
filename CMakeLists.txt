
cmake_minimum_required(VERSION 3.22)

project(koala-lang LANGUAGES C)

# verion
set(KOALA_VERSION_MAJOR 0)
set(KOALA_VERSION_MINOR 4)
set(KOALA_VERSION_PATCH 0)
set(KOALA_VERSION_MAIN "${KOALA_VERSION_MAJOR}.${KOALA_VERSION_MINOR}")
set(KOALA_VERSION "${KOALA_VERSION_MAIN}.${KOALA_VERSION_PATCH}")

configure_file(
    ${PROJECT_SOURCE_DIR}/include/version.h.in
    ${PROJECT_BINARY_DIR}/include/version.h)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local/lib)
# add_definitions(-DMAX_GC_MEM_SIZE=330)
add_definitions(-DMAX_GC_MEM_SIZE=1024)
add_definitions(-DLOG_COLOR)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_C_EXTENSIONS on)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(ENV(KOALA_PATH) ${PROJECT_BINARY_DIR}/libs)
set(ENV(LD_LIBRARY_PATH) ${PROJECT_BINARY_DIR}/lib)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL debug)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -std=gnu17 -fvar-tracking")
elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -std=gnu17 -fvar-tracking")
elseif(CMAKE_BUILD_TYPE STREQUAL release)
    add_definitions(-DNOLOG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu17 -flto -O3 -DNDEBUG -Wextra -Wno-unused-result -Wsign-compare -Wno-unused-result -Wno-unused-parameter -Wno-missing-field-initializers -Werror=implicit-function-declaration -fvisibility=hidden")
elseif(CMAKE_BUILD_TYPE STREQUAL pgo1)
    add_definitions(-DNOLOG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu17 -fprofile-generate=./profile -flto -O3 -DNDEBUG -Wextra -Wno-unused-result -Wsign-compare -Wno-unused-result -Wno-unused-parameter -Wno-missing-field-initializers -Werror=implicit-function-declaration -fvisibility=hidden")
elseif(CMAKE_BUILD_TYPE STREQUAL pgo2)
    add_definitions(-DNOLOG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu17 -fprofile-use=${CMAKE_BINARY_DIR}/test/profile -flto -O3 -DNDEBUG -Wextra -Wno-unused-result -Wsign-compare -Wno-unused-result -Wno-unused-parameter -Wno-missing-field-initializers -Werror=implicit-function-declaration -fvisibility=hidden")
elseif(CMAKE_BUILD_TYPE STREQUAL coverage)
    # lcov --rc branch_coverage.=1 -c -d ./ -o app.info
    # genhtml --rc branch_coverage.=1 app.info -o cc_result
    set(CMAKE_C_FLAGS "-g -O0 -std=gnu17 ${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
else()
    message("please specify CMAKE_BUILD_TYPE")
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/common)
include_directories(${PROJECT_BINARY_DIR}/include/common)
include_directories(${PROJECT_SOURCE_DIR}/include/parser)
include_directories(${PROJECT_BINARY_DIR}/include/parser)

add_subdirectory(src)

option(SKIP_TESTS "running test" OFF)

if(NOT SKIP_TESTS)
    enable_testing()
    add_subdirectory(test)
    add_subdirectory(test-kl)
endif()
