
# add a target to build the tests
add_executable(test_exec
    test_main.cc
    camera_tests.cc
    )
    
target_link_libraries(test_exec boost_test_exec_monitor)
#target_link_libraries(test_exec boost_unit_test_framework)

# add a target to run the tests
add_custom_target(test-all test_exec DEPENDS test_exec)
