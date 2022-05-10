# Main CMakeList for chaos project.
cmake_minimum_required(VERSION 3.15)

project(chaos VERSION 2.0
	      DESCRIPTION "Twitch Controls Chaos"
	      LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

set(CMAKE_BUILD_TYPE Debug)
set(RASPBERRY_PI FALSE)
set(USE_DUALSENSE FALSE)

# Parse version variables
file(READ "VERSION" ver)

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

configure_file(${CMAKE_CURRENT_LIST_DIR}/include/config.in ${CMAKE_CURRENT_LIST_DIR}/include/config.hpp )

# Standard CMake modules
#include(CTest)
#include(GNUInstallDirs)
include(FetchContent)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/")

# tomlplusplus and plog are header-only libraries
FetchContent_Declare(
	tomlplusplus
	GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
	GIT_TAG	       v2.5.0
)
FetchContent_Declare(
	plog
	GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
	GIT_TAG	       origin/master
)
FetchContent_Declare(
	mogi
	GIT_REPOSITORY https://github.com/mogillc/nico.git
	GIT_TAG	       origin/master
)
FetchContent_Declare(
	zmqpp
	GIT_REPOSITORY https://github.com/zeromq/zmqpp.git
	GIT_TAG	       origin/master
)
FetchContent_Declare(
	sniffify
	GIT_REPOSITORY https://github.com/polysyllabic/usb-sniffify.git
	GIT_TAG	       origin/main
)

# Life would be easier if the Raspberry OS cmake were 3.18 or greater
if(NOT mogi_POPULATED)
	FetchContent_Populate(mogi)
  add_subdirectory(${mogi_SOURCE_DIR}/edisonLibmogiPackage/libmogi)
endif()

FetchContent_MakeAvailable(tomlplusplus plog zmqpp sniffify)

find_package (Threads REQUIRED)
find_package (LibJsoncpp REQUIRED)
find_package (Libusb1 REQUIRED)

add_executable(chaos include/config.hpp
	include/chaosEngine.hpp
	include/chaosInterface.hpp
  include/commandListener.hpp
  include/commandObserver.hpp
  include/commandSender.hpp
	include/configuration.hpp
  include/controller.hpp
  include/controllerInput.hpp
	include/controllerState.hpp
  include/cooldownModifier.hpp
  include/delayModifier.hpp
  include/deviceEvent.hpp
  include/disableModifier.hpp
	include/dualshock.hpp
  include/enumerations.hpp
	include/factory.hpp
  include/formulaModifier.hpp
  include/game.hpp
  include/gameCommand.hpp
  include/gameCondition.hpp
  include/gameMenu.hpp
  include/invertModifier.hpp
  include/menuItem.hpp
  include/menuModifier.hpp
  include/menuOption.hpp
  include/menuSelect.hpp
	include/modifier.hpp
  include/parentModifier.hpp
  include/remapModifier.hpp
  include/repeatModifier.hpp
  include/sequence.hpp
  include/sequenceModifier.hpp
  include/signalRemap.hpp
  include/signals.hpp
  include/subMenu.hpp
	include/touchpad.hpp
	src/chaosEngine.cpp
	src/chaosInterface.cpp
	src/commandListener.cpp
	src/commandSender.cpp
	src/configuration.cpp
	src/controller.cpp
  src/controllerInput.cpp
	src/controllerState.cpp
  src/cooldownModifier.cpp
  src/delayModifier.cpp
  src/disableModifier.cpp
	src/dualshock.cpp
  src/formulaModifier.cpp
  src/game.cpp
	src/gameCommand.cpp
  src/gameCondition.cpp
  src/gameMenu.cpp
  src/invertModifier.cpp
  src/menuItem.cpp
  src/menuModifier.cpp
  src/menuOption.cpp
  src/menuSelect.cpp
  src/modifier.cpp
  src/parentModifier.cpp
  src/remapModifier.cpp
  src/repeatModifier.cpp
  src/sequence.cpp
  src/sequenceModifier.cpp
  src/subMenu.cpp
	src/touchpad.cpp
	src/main.cpp)

target_compile_features(chaos PRIVATE cxx_std_17)

target_include_directories (chaos PRIVATE include 
	${tomlplusplus_SOURCE_DIR}/include
	${plog_SOURCE_DIR}/include
	${mogi_SOURCE_DIR}/edisonLibmogiPackage/libmogi/include
	${zmqpp_SOURCE_DIR}/src
	${sniffify_SOURCE_DIR}/include
	${LIBJSONCPP_INCLUDE_DIRS}
	${LIBUSB1_INCLUDE_DIRS}
)

#add_subdirectory(src/sniffify)
#add_subdirectory(src/signals)
#add_subdirectory(src/controller)
#add_subdirectory(src/menu)
#add_subdirectory(src/mods)
#add_subdirectory(src/engine)

target_link_libraries(chaos	PUBLIC #chaos_controller chaos_engine chaos_menus chaos_mods
		sniffify 
		mogi 
		zmqpp 
		${LIBUSB_LIBRARIES})

if (RASPBERRY_PI)
include(ExternalProject)
# We use a patched fork insttead of re-applying the patch every time
ExternalProject_Add(raw-gadget
	GIT_REPOSITORY https://github.com/polysyllabic/raw-gadget.git
	GIT_TAG	       origin/master
  UPDATE_DISCONNECTED true
  CONFIGURE_COMMAND ""  
  BUILD_COMMAND make -j -C <SOURCE_DIR>/raw_gadget
)
endif()

#find_package(Doxygen)
#if (DOXYGEN_FOUND)
#	set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
#	set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
#	configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
#	message("Configuring Doxygen")
#
#	add_custom_target(doc ALL
#		COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
#		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#		COMMENT "Generating HTML documentation"
#		VERBATIM)
#endif()
