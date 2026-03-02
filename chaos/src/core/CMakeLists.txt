add_library(chaos_core OBJECT
  CooldownModifier.cpp
  DelayModifier.cpp
  DisableModifier.cpp
  FormulaModifier.cpp
  Game.cpp
  GameCommand.cpp
  GameCondition.cpp
  GameMenu.cpp
  MenuItem.cpp
  MenuModifier.cpp
  Modifier.cpp
  ModifierTable.cpp
  ParentModifier.cpp
  RemapModifier.cpp
  RepeatModifier.cpp
  ScalingModifier.cpp
  Sequence.cpp
  SequenceModifier.cpp
  SequenceTable.cpp
)

target_compile_features(chaos_core PRIVATE cxx_std_17)

target_include_directories(chaos_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src/core
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${tomlplusplus_SOURCE_DIR}/include
    ${plog_SOURCE_DIR}/include
    ${cppzmq_SOURCE_DIR}
    ${LIBUSB1_INCLUDE_DIRS}
)

target_link_libraries(chaos_core
  PUBLIC
    chaos_utils
    chaos_controller
    ${LIBUSB_LIBRARIES}
)
