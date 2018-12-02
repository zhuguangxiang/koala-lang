
CMAKE_MINIMUM_REQUIRED (VERSION 3.0)

PROJECT (koala-lang)

option (SCHED_LOCKED "locked scheduler" ON)
option (SWITCH_UCONTEXT "using ucontext" ON)
option (LOG_COLOR "Enable Color Log" ON)
configure_file (
	${PROJECT_SOURCE_DIR}/include/config.h.in
	${PROJECT_SOURCE_DIR}/include/config.h
	)

## ADD_DEFINITIONS (-DLOG_COLOR)

## set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__FNAME__='\"$(notdir $<)\"'")

INCLUDE_DIRECTORIES (
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/libtask
	)
SET (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
## SET (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

ADD_SUBDIRECTORY (src)
ADD_SUBDIRECTORY (libtask)
ADD_SUBDIRECTORY (test)

enable_testing()
add_test(NAME test_vector COMMAND test_vector)
add_test(NAME test_hashtable COMMAND test_hashtable)
add_test(NAME test_buffer COMMAND test_buffer)
add_test(NAME test_atomstring COMMAND test_atomstring)
add_test(NAME test_typedesc COMMAND test_typedesc)
add_test(NAME test_object COMMAND test_object)
add_test(NAME test_trait COMMAND test_trait)
add_test(NAME test_stringobject COMMAND test_stringobject)
add_test(NAME test_intobject COMMAND test_intobject)
add_test(NAME test_tupleobject COMMAND test_tupleobject)
add_test(NAME test_codeobject COMMAND test_codeobject)
add_test(NAME test_package COMMAND test_package)
add_test(NAME test_state COMMAND test_state)
add_test(NAME test_image COMMAND test_image)
add_test(NAME test_properties COMMAND test_properties)
add_test(NAME test_task COMMAND test_task)

add_test(NAME test_color COMMAND test_color)
