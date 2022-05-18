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

# GCC 8 (still in use by Raspberry Pi) requires an explicit link of libstdc++fs, the library for
# std::filesystem. This is unnecessary on GCC 9 or later
function( set_required_build_settings_for_GCC8 )
  # Note: This command makes sure that this option comes pretty late on the cmdline.
  link_libraries( "$<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:-lstdc++fs>" )
endfunction()

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
FetchContent_Declare(
	raw-gadget
	GIT_REPOSITORY https://github.com/polysyllabic/raw-gadget.git
	GIT_TAG	       origin/master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)

# Life would be easier if the Raspberry OS cmake were 3.18 or greater
if(NOT mogi_POPULATED)
	FetchContent_Populate(mogi)
  add_subdirectory(${mogi_SOURCE_DIR}/edisonLibmogiPackage/libmogi)
endif()

FetchContent_MakeAvailable(tomlplusplus plog zmqpp sniffify raw-gadget)

find_package (Threads REQUIRED)
find_package (LibJsoncpp REQUIRED)
find_package (Libusb1 REQUIRED)

# This should make the old GCC 8 compiler on the Pi do the right thing while still being ignored
# on a development system with a more modern toolchain.
set_required_build_settings_for_GCC8()

add_executable(chaos include/config.hpp
	include/ChaosEngine.hpp
	include/ChaosInterface.hpp
  include/CommandListener.hpp
  include/CommandObserver.hpp
  include/CommandSender.hpp
	include/Configuration.hpp
  include/Controller.hpp
  include/ControllerInjector.hpp
  include/ControllerInput.hpp
  include/ControllerInputTable.hpp
  include/ControllerRaw.hpp
	include/ControllerState.hpp
  include/CooldownModifier.hpp
  include/DelayModifier.hpp
  include/DeviceEvent.hpp
  include/DisableModifier.hpp
	include/Dualshock.hpp
  include/EngineInterface.hpp
  include/enumerations.hpp
	include/factory.hpp
  include/FormulaModifier.hpp
  include/Game.hpp
  include/GameCommand.hpp
  include/GameCommandTable.hpp
  include/GameCondition.hpp
  include/GameConditionTable.hpp
  include/GameMenu.hpp
  include/InvertModifier.hpp
  include/MenuInterface.hpp
  include/MenuItem.hpp
  include/MenuModifier.hpp
  include/MenuOption.hpp
  include/MenuSelect.hpp
	include/Modifier.hpp
  include/ModifierTable.hpp
  include/ParentModifier.hpp 
  include/RemapModifier.hpp
  include/RepeatModifier.hpp
  include/Sequence.hpp
  include/SequenceModifier.hpp
  include/SequenceTable.hpp
  include/signals.hpp
  include/SubMenu.hpp
  include/TOMLUtils.hpp
	include/Touchpad.hpp
	src/ChaosEngine.cpp
	src/ChaosInterface.cpp
	src/CommandListener.cpp
	src/CommandSender.cpp
	src/Configuration.cpp
	src/Controller.cpp
  src/ControllerInput.cpp
  src/ControllerInputTable.cpp
  src/ControllerRaw.cpp
	src/ControllerState.cpp
  src/CooldownModifier.cpp
  src/DelayModifier.cpp
  src/DisableModifier.cpp
	src/Dualshock.cpp
  src/FormulaModifier.cpp
  src/Game.cpp
	src/GameCommand.cpp
  src/GameCommandTable.cpp
  src/GameCondition.cpp
  src/GameConditionTable.cpp
  src/GameMenu.cpp
  src/InvertModifier.cpp
  src/MenuItem.cpp
  src/MenuModifier.cpp
  src/MenuOption.cpp
  src/MenuSelect.cpp
  src/Modifier.cpp
  src/ModifierTable.cpp
  src/ParentModifier.cpp
  src/RemapModifier.cpp
  src/RepeatModifier.cpp
  src/Sequence.cpp
  src/SequenceModifier.cpp
  src/SequenceTable.cpp
  src/SubMenu.cpp
	src/Touchpad.cpp
  src/TOMLUtils.cpp
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
