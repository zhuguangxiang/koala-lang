
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

PROJECT (koala-lang)

#set (CMAKE_BUILD_TYPE Release)
set (CMAKE_BUILD_TYPE Debug)

set (CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)

option (SCHED_LOCKED "locked scheduler" ON)
option (SWITCH_UCONTEXT "using ucontext" ON)
option (LOG_COLOR "Enable Color Log" ON)
configure_file (
  ${PROJECT_SOURCE_DIR}/include/config.h.in
  ${PROJECT_SOURCE_DIR}/include/config.h
  )

## set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -g -fprofile-arcs -ftest-coverage")
if (CMAKE_BUILD_TYPE STREQUAL Debug)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -g")
else ()
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -O2 -Wall")
endif ()
message("Build Type:" ${CMAKE_BUILD_TYPE} ${CMAKE_C_FLAGS})

## ADD_DEFINITIONS (-DLOG_COLOR)
ADD_DEFINITIONS(-fPIC)

#set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__FILENAME__='\"$(notdir $<)\"'")

INCLUDE_DIRECTORIES (
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/libtask
  )

# SET (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# SET (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

ADD_SUBDIRECTORY (src)
ADD_SUBDIRECTORY (libtask)
ADD_SUBDIRECTORY (test)

enable_testing()
add_test(NAME test_vector COMMAND test_vector)
add_test(NAME test_hashtable COMMAND test_hashtable)
add_test(NAME test_buffer COMMAND test_buffer)
add_test(NAME test_stringbuf COMMAND test_stringbuf)
add_test(NAME test_atomstring COMMAND test_atomstring)
add_test(NAME test_typedesc COMMAND test_typedesc)
add_test(NAME test_pkgnode COMMAND test_pkgnode)
add_test(NAME test_object COMMAND test_object)
add_test(NAME test_trait COMMAND test_trait)
add_test(NAME test_stringobject COMMAND test_stringobject)
add_test(NAME test_intobject COMMAND test_intobject)
add_test(NAME test_tupleobject COMMAND test_tupleobject)
add_test(NAME test_codeobject COMMAND test_codeobject)
add_test(NAME test_package COMMAND test_package)
add_test(NAME test_image COMMAND test_image)
add_test(NAME test_properties COMMAND test_properties)
add_test(NAME test_task COMMAND test_task)
add_test(NAME test_join COMMAND test_join)
add_test(NAME test_color COMMAND test_color)
