
# add a target to build the tests
add_executable(test_exec
    linalg_ref_test.cc
    v3_test.cc
    )
    
target_link_libraries(test_exec boost_test_exec_monitor)

# add a target to run the tests
add_custom_target(test-all test_exec DEPENDS test_exec)
