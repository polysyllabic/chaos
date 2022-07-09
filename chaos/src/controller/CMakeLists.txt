
add_library (chaos_controller 
  Controller.cpp
  Controller.hpp
  ControllerInjector.hpp
  ControllerInput.cpp
  ControllerInput.hpp
  ControllerInputTable.cpp
  ControllerInputTable.hpp
  ControllerRaw.cpp
  ControllerRaw.hpp
  ControllerState.cpp
  ControllerState.hpp
  DeviceEvent.hpp
  Dualshock.cpp
  Dualshock.hpp
  signals.hpp
  Touchpad.hpp
  Touchpad.cpp
  )

target_compile_features(chaos_controller PRIVATE cxx_std_17)

target_include_directories(chaos_controller
  PRIVATE
  	${plog_SOURCE_DIR}/include
    ${tomlplusplus_SOURCE_DIR}/include
    ${chaos_utils_SOURCE_DIR}
    ${sniffify_SOURCE_DIR}/include
    ${LIBUSB1_INCLUDE_DIRS}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chaos_controller 
    chaos_utils
    sniffify
    ${LIBUSB_LIBRARIES}
)
