
find_program(FILECHECK_EXE NAMES filecheck FileCheck REQUIRED)

file(GLOB KL_TEST_FILES "*.kl")

foreach(test_file ${KL_TEST_FILES})
    get_filename_component(test_name "${test_file}" NAME_WE)
    get_filename_component(test_dir "${test_file}" DIRECTORY)

    # Skip bench tests in regular test run
    if("${test_dir}" MATCHES "test-bench")
        continue()
    endif()

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

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg
    ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
    @ONLY)

# add a target to run the test, build this test will trigger the run
add_custom_target(lit-tests
    COMMAND lit ${CMAKE_CURRENT_BINARY_DIR} -vv --show-xfail --vg-leak
    DEPENDS koalac_out
    COMMENT "Running Koala lit test suite...")

# C unit tests, one executable per common module, registered as c_<module>
add_subdirectory(test-c)

get_property(c_test_targets GLOBAL PROPERTY KOALA_C_TEST_TARGETS)

if(c_test_targets)
    # Define a custom target to run all C unit tests via 'cmake --build . --target koala-c-tests'
    add_custom_target(koala-c-tests
        COMMAND ${CMAKE_CTEST_COMMAND} -R "^c_" --output-on-failure
        DEPENDS ${c_test_targets}
        COMMENT "Executing Koala C unit test suite...")
endif()

# why gone?
