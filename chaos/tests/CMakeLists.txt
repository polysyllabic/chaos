
# These are integration tests (do not add to unit test list) and run in conjunction with a python
# test to ensure that both sides of the zmq socket can talk to each other
add_executable(test_socket test_sockets.cpp)
target_include_directories(test_socket PRIVATE chaos_communicator)
target_link_libraries(test_socket chaos_communicator)


