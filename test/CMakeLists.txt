
add_executable(test_vector test_vector.c)
target_link_libraries(test_vector koala-common)

add_executable(test_hashtable test_hashtable.c)
target_link_libraries(test_hashtable koala-common)

add_executable(test_buffer test_buffer.c)
target_link_libraries(test_buffer koala-common)

add_executable(test_stringbuf test_stringbuf.c)
target_link_libraries(test_stringbuf koala-common)

add_executable(test_atomstring test_atomstring.c)
target_link_libraries(test_atomstring koala-common)

add_executable(test_typedesc test_typedesc.c)
target_link_libraries(test_typedesc koala-common)

add_executable(test_pkgnode test_pkgnode.c)
target_link_libraries(test_pkgnode koala-common)

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

add_executable(test_properties test_properties.c)
target_link_libraries(test_properties koala-common)

add_executable(test_task test_task.c)
target_link_libraries(test_task koala-task)

add_executable(test_join test_join.c)
target_link_libraries(test_join koala-task)

add_executable(test_color test_color.c)
