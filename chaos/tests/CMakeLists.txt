
# Integration tests (do not add to unit test list)
add_executable(test_socket test_sockets.cpp)
target_include_directories(test_socket PRIVATE chaos_communicator)
target_link_libraries(test_socket chaos_communicator)

add_executable(test_menu)
target_sources(test_menu PRIVATE test_menu_sequences.cpp
  ../src/GameMenu.cpp
  ../src/MenuItem.cpp
  ../src/Sequence.cpp
  ../src/SequenceTable.cpp
)
  
target_include_directories(test_menu PRIVATE ../include
  ../src/controller
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include)

target_link_libraries(test_menu PRIVATE chaos_controller)

add_executable(test_remapping)
target_sources(test_remapping PRIVATE
  test_remapping.cpp
  ../src/Modifier.cpp
  ../src/RemapModifier.cpp
  ../src/Sequence.cpp
  ../src/GameCondition.cpp
)
target_include_directories(test_remapping PRIVATE
  ../include
  ../src/controller
  ../src/utils
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
)
target_link_libraries(test_remapping PRIVATE chaos_controller chaos_utils)

add_executable(test_engine_lifecycle)
target_sources(test_engine_lifecycle PRIVATE
  test_engine_lifecycle.cpp
  ../src/ChaosEngine.cpp
  ../src/Game.cpp
  ../src/GameCommand.cpp
  ../src/GameCondition.cpp
  ../src/GameMenu.cpp
  ../src/MenuItem.cpp
  ../src/Modifier.cpp
  ../src/ModifierTable.cpp
  ../src/Sequence.cpp
  ../src/SequenceTable.cpp
)
target_include_directories(test_engine_lifecycle PRIVATE
  ../include
  ../src/controller
  ../src/communicator
  ../src/utils
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
  ${cppzmq_SOURCE_DIR}
)
target_link_libraries(test_engine_lifecycle PRIVATE
  chaos_controller
  chaos_communicator
  chaos_utils
)
