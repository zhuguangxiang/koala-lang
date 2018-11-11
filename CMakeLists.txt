
CMAKE_MINIMUM_REQUIRED (VERSION 3.0)

PROJECT (koala-lang)

option (LOG_COLOR "Enable Color Log" ON)
configure_file (${PROJECT_SOURCE_DIR}/include/config.h.in
                ${PROJECT_SOURCE_DIR}/include/config.h)

## ADD_DEFINITIONS (-DLOG_COLOR)

## set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__FNAME__='\"$(notdir $<)\"'")

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/include)
SET (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

ADD_SUBDIRECTORY (src)
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
