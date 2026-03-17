
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

set(USB_SNIFFIFY_HARDWARE_TEST_SUPPORT
  UsbSniffifyHardwareTestUtils.cpp
)

function(add_usb_sniffify_hardware_test target_name source_name)
  add_executable(${target_name}
    ${source_name}
    ${USB_SNIFFIFY_HARDWARE_TEST_SUPPORT}
  )
  target_compile_features(${target_name} PRIVATE cxx_std_17)
  target_link_libraries(${target_name} PRIVATE chaos_usb_transport)
endfunction()

# Pi-only hardware smoke tests for usb_sniffify. These are intentionally separate
# from the generic unit-test script because they require Raspberry Pi USB hardware
# and, for live raw-gadget access, root privileges.
add_usb_sniffify_hardware_test(test_usb_sniffify_environment test_usb_sniffify_environment.cpp)
add_usb_sniffify_hardware_test(test_usb_sniffify_raw_gadget_session test_usb_sniffify_raw_gadget_session.cpp)
add_usb_sniffify_hardware_test(test_usb_sniffify_passthrough_smoke test_usb_sniffify_passthrough_smoke.cpp)
