add_library(chaos_engine OBJECT
  ChaosEngine.cpp
  Configuration.cpp
)

target_compile_features(chaos_engine PRIVATE cxx_std_17)

target_include_directories(chaos_engine
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src/engine
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${tomlplusplus_SOURCE_DIR}/include
    ${plog_SOURCE_DIR}/include
    ${cppzmq_SOURCE_DIR}
    ${LIBUSB1_INCLUDE_DIRS}
)

target_link_libraries(chaos_engine
  PUBLIC
    chaos_core
    chaos_communicator
    chaos_utils
    chaos_controller
    ${LIBUSB_LIBRARIES}
)
