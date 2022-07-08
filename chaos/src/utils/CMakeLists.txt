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
)
target_include_directories(chaos_utils PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)