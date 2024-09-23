
configure_file(lit.site.cfg lit.cfg @ONLY)

# add a target to run the test, build this test will trigger the run
add_custom_target(check
    COMMAND lit ${CMAKE_CURRENT_BINARY_DIR} -vv --show-xfail --vg-leak)

# why gone?
