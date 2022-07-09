find_package (Threads REQUIRED)

add_library (chaos_utils
  random.cpp
  random.hpp
  timer.cpp
  timer.hpp
  thread.cpp
  thread.hpp
  jsoncpp.cpp
  json/json.h
  json/json-forwards.h
  TOMLUtils.cpp
  TOMLUtils.hpp
)

target_compile_features(chaos_utils PRIVATE cxx_std_17)

target_include_directories(chaos_utils
  PRIVATE
    ${tomlplusplus_SOURCE_DIR}/include
    ${plog_SOURCE_DIR}/include
  PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(chaos_utils
  ${CMAKE_THREAD_LIBS_INIT})