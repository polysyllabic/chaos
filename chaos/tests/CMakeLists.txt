
# Integration tests (do not add to unit test list)
add_executable(test_socket test_sockets.cpp)
target_include_directories(test_socket PRIVATE chaos_communicator)
target_link_libraries(test_socket chaos_communicator)

add_executable(test_menu)
target_sources(test_menu PRIVATE test_menu_sequences.cpp
  ../src/GameMenu.cpp
  ../src/MenuItem.cpp
  ../src/Sequence.cpp
  KeyboardController.cpp
  KeyboardController.hpp
)
  
target_include_directories(test_menu PRIVATE ../include
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include)

  add_executable(test_remapping)
  target_sources(test_remapping PRIVATE test_remapping.cpp)
  target_include_directories(test_remapping PRIVATE
    chaos_controller
    chaos_utils
    ${tomlplusplus_SOURCE_DIR}/include
  )
  target_link_libraries(test_remapping chaos_controller chaos_utils)