# Main CMakeList for chaos project.
cmake_minimum_required(VERSION 3.15)

project(chaos VERSION 2.0
	      DESCRIPTION "Twitch Controls Chaos"
	      LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif() 

set(CMAKE_BUILD_TYPE Debug)
set(RASPBERRY_PI TRUE)
set(USE_DUALSENSE FALSE)

# Install root for the Chaos runtime bundle.
# This is intentionally an absolute path so the systemd unit can rely on it.
set(CHAOS_INSTALL_ROOT "/usr/local/chaos"
    CACHE PATH "Absolute install directory for Chaos (used by chaos.service)")
set(CHAOS_TARGET_PLATFORM ""
    CACHE STRING "Detected or selected Raspberry Pi target platform (pi4, pi5, raspberry-pi, unknown)")
set(CHAOS_TARGET_OS_CODENAME ""
    CACHE STRING "Detected or selected target OS codename")
set(CHAOS_DEFAULT_UDC_DEVICE ""
    CACHE STRING "Default Raw Gadget UDC device name")
set(CHAOS_DEFAULT_UDC_DRIVER ""
    CACHE STRING "Default Raw Gadget UDC driver name")

# Optional local test bundle (outside systemd install path)
option(install_test "Stage a test copy of chaos under ~/test for local runs" OFF)
set(CHAOS_TEST_ROOT "$ENV{HOME}/test"
    CACHE PATH "Destination root for install_test staging")

# Parse version variables from the repository root shared VERSION file.
file(READ "${CMAKE_CURRENT_LIST_DIR}/../VERSION" ver)

if (${ver} MATCHES "VERSION_MAJOR[ \t]+([0-9]+)")
    set(CHAOS_MAJOR ${CMAKE_MATCH_1})
else()
    message( FATAL_ERROR "missing VERSION_MAJOR number" )
endif()

if(${ver} MATCHES "VERSION_MINOR[ \t]+([0-9]+)")
    set(CHAOS_MINOR ${CMAKE_MATCH_1})
else()
    message( FATAL_ERROR "missing VERSION_MINOR number" )
endif()

if(${ver} MATCHES "VERSION_PATCH[ \t]+([0-9]+)")
    set(CHAOS_PATCH ${CMAKE_MATCH_1})
else()
    message( FATAL_ERROR "missing VERSION_PATCH number" )
endif()

set(CHAOS_VERSION ${CHAOS_MAJOR}.${CHAOS_MINOR}.${CHAOS_PATCH})

if(${ver} MATCHES "PRERELEASE[ \t]+([^\n]+)")
    set(CHAOS_VERSION ${CHAOS_VERSION}-${CMAKE_MATCH_1})
    if(${ver} MATCHES "PRERELEASE_VERSION[ \t]+([0-9]+)")
        set(CHAOS_VERSION ${CHAOS_VERSION}.${CMAKE_MATCH_1})
    else()
        message( FATAL_ERROR "missing PRERELEASE_VERSION number" )
    endif()
endif()
message(NOTICE "Configuring Chaos engine ${CHAOS_VERSION}")

function(chaos_detect_target_platform out_var)
  set(_platform "unknown")
  if(EXISTS "/proc/device-tree/model")
    execute_process(
      COMMAND /bin/bash -lc "tr -d '\\0' </proc/device-tree/model"
      OUTPUT_VARIABLE _model
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(_model MATCHES "Raspberry Pi 5|Raspberry Pi 500|Compute Module 5")
      set(_platform "pi5")
    elseif(_model MATCHES "Raspberry Pi 4|Raspberry Pi 400|Compute Module 4")
      set(_platform "pi4")
    elseif(_model MATCHES "Raspberry Pi")
      set(_platform "raspberry-pi")
    endif()
  endif()
  set(${out_var} "${_platform}" PARENT_SCOPE)
endfunction()

function(chaos_detect_os_codename out_var)
  set(_codename "unknown")
  if(EXISTS "/etc/os-release")
    file(STRINGS "/etc/os-release" _os_release_lines REGEX "^(VERSION_CODENAME|DEBIAN_CODENAME)=")
    foreach(_line IN LISTS _os_release_lines)
      if(_line MATCHES "^VERSION_CODENAME=(.+)$")
        set(_codename "${CMAKE_MATCH_1}")
        break()
      elseif(_line MATCHES "^DEBIAN_CODENAME=(.+)$")
        set(_codename "${CMAKE_MATCH_1}")
      endif()
    endforeach()
    string(REPLACE "\"" "" _codename "${_codename}")
  endif()
  set(${out_var} "${_codename}" PARENT_SCOPE)
endfunction()

function(chaos_detect_default_udc out_var)
  set(_udc "")
  file(GLOB _udc_paths LIST_DIRECTORIES true "/sys/class/udc/*")
  set(_udc_names "")
  foreach(_udc_path IN LISTS _udc_paths)
    if(EXISTS "${_udc_path}")
      get_filename_component(_udc_name "${_udc_path}" NAME)
      list(APPEND _udc_names "${_udc_name}")
    endif()
  endforeach()
  list(SORT _udc_names)

  if(CHAOS_TARGET_PLATFORM STREQUAL "pi5")
    list(FIND _udc_names "1000480000.usb" _preferred_index)
    if(NOT _preferred_index EQUAL -1)
      set(_udc "1000480000.usb")
    endif()
  elseif(CHAOS_TARGET_PLATFORM STREQUAL "pi4")
    list(FIND _udc_names "fe980000.usb" _preferred_index)
    if(NOT _preferred_index EQUAL -1)
      set(_udc "fe980000.usb")
    endif()
  endif()

  if(_udc STREQUAL "" AND _udc_names)
    list(GET _udc_names 0 _udc)
  endif()

  if(_udc STREQUAL "")
    if(CHAOS_TARGET_PLATFORM STREQUAL "pi5")
      set(_udc "1000480000.usb")
    elseif(CHAOS_TARGET_PLATFORM STREQUAL "pi4")
      set(_udc "fe980000.usb")
    endif()
  endif()

  set(${out_var} "${_udc}" PARENT_SCOPE)
endfunction()

chaos_detect_target_platform(_chaos_detected_target_platform)
chaos_detect_os_codename(_chaos_detected_os_codename)

if(CHAOS_TARGET_PLATFORM STREQUAL "")
  set(CHAOS_TARGET_PLATFORM "${_chaos_detected_target_platform}"
      CACHE STRING "Detected or selected Raspberry Pi target platform (pi4, pi5, raspberry-pi, unknown)" FORCE)
endif()

if(CHAOS_TARGET_OS_CODENAME STREQUAL "")
  set(CHAOS_TARGET_OS_CODENAME "${_chaos_detected_os_codename}"
      CACHE STRING "Detected or selected target OS codename" FORCE)
endif()

chaos_detect_default_udc(_chaos_detected_default_udc)

if(CHAOS_DEFAULT_UDC_DEVICE STREQUAL "")
  set(CHAOS_DEFAULT_UDC_DEVICE "${_chaos_detected_default_udc}"
      CACHE STRING "Default Raw Gadget UDC device name" FORCE)
endif()

if(CHAOS_DEFAULT_UDC_DRIVER STREQUAL "")
  set(CHAOS_DEFAULT_UDC_DRIVER "${CHAOS_DEFAULT_UDC_DEVICE}"
      CACHE STRING "Default Raw Gadget UDC driver name" FORCE)
endif()

message(STATUS "Chaos target platform: ${CHAOS_TARGET_PLATFORM}")
message(STATUS "Chaos target OS codename: ${CHAOS_TARGET_OS_CODENAME}")
message(STATUS "Chaos default Raw Gadget UDC: device='${CHAOS_DEFAULT_UDC_DEVICE}' driver='${CHAOS_DEFAULT_UDC_DRIVER}'")

# GCC 8 requires an explicit link of libstdc++fs, the library for std::filesystem.
# This is unnecessary on GCC 9 or later.
function( set_required_build_settings_for_GCC8 )
  # Note: This command makes sure that this option comes pretty late on the cmdline.
  link_libraries( "$<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:-lstdc++fs>" )
endfunction()

configure_file(${CMAKE_CURRENT_LIST_DIR}/include/config.in ${CMAKE_CURRENT_LIST_DIR}/include/config.hpp )

# Standard CMake modules
#include(CTest)
include(GNUInstallDirs)
include(FetchContent)
include(ExternalProject)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")

# Raw gadget kernel module; This is a makefile external project
find_program(MAKE_EXECUTABLE NAMES gmake make mingw32-make REQUIRED)

# Header-only libraries
FetchContent_Declare(
	tomlplusplus
	GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
	GIT_TAG	       origin/master
)
FetchContent_Declare(
	plog
	GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
	GIT_TAG	       origin/master
)
# Ordinary libraries
FetchContent_Declare(
	cppzmq
	GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
	GIT_TAG	       origin/master
)

FetchContent_MakeAvailable(tomlplusplus plog cppzmq)

find_package (Threads REQUIRED)
find_package (Libusb1 REQUIRED)

# This should make the old GCC 8 compiler on the Pi do the right thing while still being ignored
# on a development system with a more modern toolchain.
set_required_build_settings_for_GCC8()

add_subdirectory(src/utils)
add_subdirectory(src/communicator)
add_subdirectory(src/controller)
add_subdirectory(src/core)
add_subdirectory(src/engine)

add_executable(chaos
  src/main.cpp
)
target_compile_features(chaos PRIVATE cxx_std_17)
target_include_directories (chaos PRIVATE
  include
  src/core
  src/engine
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
  ${cppzmq_SOURCE_DIR}
  ${LIBUSB1_INCLUDE_DIRS}
)
target_link_libraries(chaos
  PUBLIC
    chaos_engine
    chaos_core
)

add_executable(chaos_parse_game_config
  utils/src/parse_game_config.cpp
)
target_compile_features(chaos_parse_game_config PRIVATE cxx_std_17)
target_include_directories(chaos_parse_game_config PRIVATE
  include
  src/core
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
  ${cppzmq_SOURCE_DIR}
  ${LIBUSB1_INCLUDE_DIRS}
)
target_link_libraries(chaos_parse_game_config
  PUBLIC
    chaos_core
)

add_executable(validate_mod
  utils/src/validate_mod.cpp
)
target_compile_features(validate_mod PRIVATE cxx_std_17)
target_include_directories(validate_mod PRIVATE
  include
  src/core
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
  ${cppzmq_SOURCE_DIR}
  ${LIBUSB1_INCLUDE_DIRS}
)
target_link_libraries(validate_mod
  PUBLIC
    chaos_core
)

add_executable(gamepad_test
  utils/src/gamepad_test.cpp
)
target_compile_features(gamepad_test PRIVATE cxx_std_17)
target_include_directories(gamepad_test PRIVATE
  include
  ${tomlplusplus_SOURCE_DIR}/include
  ${plog_SOURCE_DIR}/include
  ${cppzmq_SOURCE_DIR}
  ${LIBUSB1_INCLUDE_DIRS}
)
target_link_libraries(gamepad_test
  PUBLIC
    chaos_utils
    chaos_controller
    ${LIBUSB1_LIBRARIES}
)

install(TARGETS chaos RUNTIME DESTINATION "${CHAOS_INSTALL_ROOT}")
install(TARGETS chaos_parse_game_config RUNTIME DESTINATION "${CHAOS_INSTALL_ROOT}")
install(TARGETS validate_mod RUNTIME DESTINATION "${CHAOS_INSTALL_ROOT}")
install(TARGETS gamepad_test RUNTIME DESTINATION "${CHAOS_INSTALL_ROOT}")
install(FILES chaosconfig.toml DESTINATION "${CHAOS_INSTALL_ROOT}")
install(DIRECTORY examples/ DESTINATION "${CHAOS_INSTALL_ROOT}/games")

if(install_test)
  set(_install_test_in_all FALSE)
  if(CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug$")
    set(_install_test_in_all TRUE)
  endif()

  if(_install_test_in_all)
    set(_install_test_all_keyword ALL)
  else()
    set(_install_test_all_keyword)
  endif()

  add_custom_target(install_test ${_install_test_all_keyword}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CHAOS_TEST_ROOT}/games"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:chaos>" "${CHAOS_TEST_ROOT}/chaos"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/chaosconfig.toml"
            "${CHAOS_TEST_ROOT}/chaosconfig.toml"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/examples"
            "${CHAOS_TEST_ROOT}/games"
    DEPENDS chaos
    COMMENT "Staging local test runtime at ${CHAOS_TEST_ROOT}"
    VERBATIM
  )
endif()

find_package(Doxygen)
if (DOXYGEN_FOUND)
  set(LATEX_OUTPUT NO)
  find_package(LATEX)
  if (LATEX_FOUND)
    set(LATEX_OUTPUT YES)
  endif()
	set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
	set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile)
	set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc)
	configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
	# Normalize the generated configuration for the doxygen version installed on this system.
	execute_process(
	  COMMAND ${DOXYGEN_EXECUTABLE} -u ${DOXYGEN_OUT}
	  OUTPUT_QUIET
	  ERROR_QUIET
	  RESULT_VARIABLE _DOXYGEN_UPDATE_RESULT
	)
	if (NOT _DOXYGEN_UPDATE_RESULT EQUAL 0)
	  message(WARNING "doxygen -u failed for ${DOXYGEN_OUT} (code=${_DOXYGEN_UPDATE_RESULT})")
	endif()
	message("Configuring Doxygen")

	add_custom_target(doc ALL
		COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating documentation"
		VERBATIM)
endif()

# Tests
add_subdirectory(tests EXCLUDE_FROM_ALL)
