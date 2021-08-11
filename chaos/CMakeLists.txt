# Main CMakeLists for chaos project.
cmake_minimum_required(VERSION 3.14)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

project(chaos)

# Main flags to control the build
set(CMAKE_BUILD_TYPE Debug)
set(RASPBERRY_PI TRUE)
# DualSense support is not yet ready for primetime
set(USE_DUALSENSE FALSE)
set(USE_DUALSHOCK TRUE)

target_compile_features(chaos PRIVATE cxx_std_17)

# Standard CMake modules
#include(CTest)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")


add_subdirectory( external/nico/edisonLibmogiPackage/libmogi )
include_directories( nico/edisonLibmogiPackage/libmogi/include )
set(LIBS ${LIBS} mogi)

add_subdirectory(zmqpp)
target_include_directories(chaos PUBLIC  zmqpp/src )
set(LIBS ${LIBS} zmqpp zmq)

add_subdirectory(usb-sniffify)
include_directories( usb-sniffify/include )
set(LIBS ${LIBS} sniffify)

find_package (Threads REQUIRED)
set(LIBS ${LIBS} pthread)

find_package (LibJsoncpp REQUIRED)
include_directories(${LIBJSONCPP_INCLUDE_DIRS})
set(LIBS ${LIBS} jsoncpp)

find_package (Libusb1 REQUIRED)
target_link_libraries(Libusb1)

if(RASPBERRY_PI)
	set(LIBS ${LIBS} crypt rt )
endif()

add_subdirectory (controller)
add_subdirectory (engine)
add_subdirectory (modifiers)

add_executable(chaos src/chaos.hpp
		     src/main.cpp)

target_link_libraries(chaos PRIVATE controller engine modifiers)

