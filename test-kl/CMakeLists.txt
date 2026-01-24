
find_program(FILECHECK_EXE NAMES filecheck FileCheck REQUIRED)

file(GLOB KL_TEST_FILES "*.kl")

foreach(test_file ${KL_TEST_FILES})
    get_filename_component(test_name "${test_file}" NAME_WE)

    add_test(
        NAME "${test_name}"
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh"
                "$<TARGET_FILE:koalac_out>"
                "${FILECHECK_EXE}"
                "${test_file}"
    )

    set_tests_properties("${test_name}" PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endforeach()

# Define a custom target to run all tests via 'cmake --build . --target koala-test'
add_custom_target(koala-test
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^test_" --output-on-failure
    DEPENDS koalac_out
    COMMENT "Executing Koala integration test suite..."
)
