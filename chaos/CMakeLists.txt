# Main CMakeLists for chaos project.
cmake_minimum_required(VERSION 3.18)

project(chaos VERSION 2.0
	      DESCRIPTION "Twitch Controls Chaos"
	      LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

set(CMAKE_BUILD_TYPE Debug)
set(RASPBERRY_PI TRUE)
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

configure_file(${CMAKE_CURRENT_LIST_DIR}/include/config.hpp.in ${CMAKE_CURRENT_LIST_DIR}/include/config.hpp )

# Standard CMake modules
#include(CTest)
include(GNUInstallDirs)
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
	SOURCE_SUBDIR  edisonLibmogiPackage/libmogi
)
FetchContent_Declare(
	zmqpp
	GIT_REPOSITORY https://github.com/zeromq/zmqpp.git
	GIT_TAG	       origin/master
)
FetchContent_Declare(
	sniffify
	GIT_REPOSITORY https://github.com/blegas78/usb-sniffify.git
	GIT_TAG	       origin/main
)
FetchContent_Declare(
	raw-gadget
	GIT_REPOSITORY https://github.com/polysyllabic/raw-gadget.git
	GIT_TAG	       origin/master
)

FetchContent_MakeAvailable(tomlplusplus plog mogi zmqpp sniffify raw-gadget)

#add_subdirectory(${mogi_SOURCE_DIR}/edisonLibmogiPackage/libmogi)

find_package (Threads REQUIRED)
find_package (LibJsoncpp REQUIRED)
find_package (Libusb1 REQUIRED)

add_executable(chaos include/config.hpp
		     include/chaosEngine.hpp
		     src/chaosEngine.cpp
		     include/chaosInterface.hpp
		     src/chaosInterface.cpp
		     include/chaosUhid.hpp
		     src/chaosUhid.cpp
		     src/commandListener.cpp
		     src/commandSender.cpp
		     include/controller.hpp
		     src/controller.cpp
		     include/controllerRaw.hpp
		     src/controllerRaw.cpp
		     include/controllerState.hpp
		     src/controllerState.cpp
		     include/dualshock.hpp
		     src/dualshock.cpp
		     include/deviceTypes.hpp
		     include/controllerCommand.hpp
		     src/controllerCommand.cpp
		     include/gameCommand.hpp
		     src/gameCommand.cpp
		     include/modifierFactory.hpp
		     include/modifier.hpp
		     src/modifier.cpp
		     include/cooldownModifier.hpp
		     src/cooldownModifier.cpp
		     include/delayModifier.hpp
		     src/delayModifier.cpp
		     include/disableModifier.hpp
		     src/disableModifier.cpp
		     include/formulaModifier.hpp
		     src/formulaModifier.cpp
		     include/menuModifier.hpp
		     src/menuModifier.cpp
		     include/parentModifier.hpp
		     src/parentModifier.cpp
		     include/remapModifier.hpp
		     src/remapModifier.cpp
		     include/repeatModifier.hpp
		     src/repeatModifier.cpp
		     include/sequenceModifier.hpp
		     src/sequenceModifier.cpp
		     include/sequence.hpp
		     src/sequence.cpp
		     src/sequenceAbsolute.cpp
		     src/sequenceRelative.cpp
		     include/tomlReader.hpp
		     src/tomlReader.cpp
		     src/main.cpp)


if(USE_DUALSENSE)
	add_target_sources(chaos PUBLIC include/dualsense.hpp
				 	src/dualsense.cpp)
endif()

target_compile_features(chaos PRIVATE cxx_std_17)

target_include_directories (chaos PUBLIC include 
			   	  	 ${tomlplusplus_SOURCE_DIR}/include
			   	  	 ${plog_SOURCE_DIR}/include
					 ${mogi_SOURCE_DIR}/edisonLibmogiPackage/libmogi/include
					 ${zmqpp_SOURCE_DIR}/src
					 ${sniffify_SOURCE_DIR}/include
					 ${LIBJSONCPP_INCLUDE_DIRS}
					 ${LIBUSB1_INCLUDE_DIRS})

target_link_libraries(chaos PUBLIC sniffify mogi zmqpp)

