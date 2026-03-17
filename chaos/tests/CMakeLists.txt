
# Integration tests (do not add to unit test list)
add_executable(test_socket test_sockets.cpp)
target_include_directories(test_socket PRIVATE chaos_communicator)
target_link_libraries(test_socket chaos_communicator)

add_executable(test_menu)
target_sources(test_menu PRIVATE test_menu_sequences.cpp)
  
target_include_directories(test_menu PRIVATE ../include
  ../src/core
  ../src/engine
  ../src/controller
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include)

target_link_libraries(test_menu PRIVATE chaos_core)

add_executable(test_remapping)
target_sources(test_remapping PRIVATE
  test_remapping.cpp
)
target_include_directories(test_remapping PRIVATE
  ../include
  ../src/core
  ../src/engine
  ../src/controller
  ../src/utils
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
)
target_link_libraries(test_remapping PRIVATE chaos_core)

add_executable(test_modifier_types)
target_sources(test_modifier_types PRIVATE
  test_modifier_types.cpp
)
target_include_directories(test_modifier_types PRIVATE
  ../include
  ../src/core
  ../src/engine
  ../src/controller
  ../src/utils
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
)
target_link_libraries(test_modifier_types PRIVATE chaos_core)

add_executable(test_tlou2_modifier_coverage)
target_sources(test_tlou2_modifier_coverage PRIVATE
  test_tlou2_modifier_coverage.cpp
)
target_include_directories(test_tlou2_modifier_coverage PRIVATE
  ../include
  ../src/core
  ../src/engine
  ../src/controller
  ../src/utils
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
)
target_link_libraries(test_tlou2_modifier_coverage PRIVATE chaos_core)

add_executable(test_engine_lifecycle)
target_sources(test_engine_lifecycle PRIVATE
  test_engine_lifecycle.cpp
)
target_include_directories(test_engine_lifecycle PRIVATE
  ../include
  ../src/core
  ../src/engine
  ../src/controller
  ../src/communicator
  ../src/utils
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
  ${cppzmq_SOURCE_DIR}
)
target_link_libraries(test_engine_lifecycle PRIVATE
  chaos_engine
  chaos_core
)

# Hardware probe helper: prints VID/PID for the controller detected on any available USB port.
add_executable(probe_controller_vidpid probe_controller_vidpid.cpp)
target_link_libraries(probe_controller_vidpid PRIVATE chaos_usb_transport)
