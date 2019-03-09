
add_executable(test_vector test_vector.c)
target_link_libraries(test_vector common)

add_executable(test_hashtable test_hashtable.c)
target_link_libraries(test_hashtable common)

add_executable(test_buffer test_buffer.c)
target_link_libraries(test_buffer common)

add_executable(test_stringbuf test_stringbuf.c)
target_link_libraries(test_stringbuf common)

add_executable(test_atomstring test_atomstring.c)
target_link_libraries(test_atomstring common)

add_executable(test_typedesc test_typedesc.c)
target_link_libraries(test_typedesc common)

add_executable(test_pkgnode test_pkgnode.c)
target_link_libraries(test_pkgnode common)

add_executable(test_object test_object.c)
target_link_libraries(test_object koala)

add_executable(test_trait test_trait.c)
target_link_libraries(test_trait koala)

add_executable(test_stringobject test_stringobject.c)
target_link_libraries(test_stringobject koala)

add_executable(test_intobject test_intobject.c)
target_link_libraries(test_intobject koala)

add_executable(test_tupleobject test_tupleobject.c)
target_link_libraries(test_tupleobject koala)

add_executable(test_codeobject test_codeobject.c)
target_link_libraries(test_codeobject koala)

add_executable(test_package test_package.c)
target_link_libraries(test_package koala)

add_executable(test_image test_image.c)
target_link_libraries(test_image koala)

add_executable(test_io test_io.c)
target_link_libraries(test_io koala)

add_executable(test_properties test_properties.c)
target_link_libraries(test_properties common)

add_executable(test_task test_task.c)
target_link_libraries(test_task task)

add_executable(test_join test_join.c)
target_link_libraries(test_join task)
