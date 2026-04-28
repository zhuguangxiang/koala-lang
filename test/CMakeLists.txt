
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

# Define a custom target to run all tests via 'cmake --build . --target koala-tests'
add_custom_target(koala-tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^test_" --output-on-failure
    DEPENDS koalac_out
    COMMENT "Executing Koala integration test suite..."
)

configure_file(lit.site.cfg lit.cfg @ONLY)

# add a target to run the test, build this test will trigger the run
add_custom_target(lit-tests
    COMMAND lit ${CMAKE_CURRENT_BINARY_DIR} -vv --show-xfail --vg-leak
    DEPENDS koalac_out
    COMMENT "Running Koala lit test suite...")

# why gone?
