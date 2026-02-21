add_library(chaos_usb_transport STATIC EXCLUDE_FROM_ALL
  src/raw-helper.cpp
  src/raw-gadget.cpp
  src/raw-gadget-passthrough.cpp
  include/raw-gadget.hpp
  include/raw-helper.h
)

target_compile_features(chaos_usb_transport PRIVATE cxx_std_17)

target_include_directories(chaos_usb_transport
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBUSB1_INCLUDE_DIRS}
    ${plog_SOURCE_DIR}/include
)

target_link_libraries(chaos_usb_transport
  PUBLIC
    ${LIBUSB_LIBRARIES}
    Threads::Threads
)
