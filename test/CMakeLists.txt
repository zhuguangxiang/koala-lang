
add_executable(test_vector test_vector.c)
target_link_libraries(test_vector koala)

add_executable(test_hashtable test_hashtable.c)
target_link_libraries(test_hashtable koala)

add_executable(test_buffer test_buffer.c)
target_link_libraries(test_buffer koala)

add_executable(test_atomstring test_atomstring.c)
target_link_libraries(test_atomstring koala)

add_executable(test_typedesc test_typedesc.c)
target_link_libraries(test_typedesc koala)

add_executable(test_object test_object.c)
target_link_libraries(test_object koala)

add_executable(test_trait test_trait.c)
target_link_libraries(test_trait koala)

add_executable(test_stringobject test_stringobject.c)
target_link_libraries(test_stringobject koala)

add_executable(test_intobject test_intobject.c)
target_link_libraries(test_intobject koala)
