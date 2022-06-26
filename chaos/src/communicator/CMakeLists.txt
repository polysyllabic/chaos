
add_library (chaos_communicator 
    ChaosInterface.cpp
    CommandListener.cpp
    CommandSender.cpp
  	CommandListener.hpp
    CommandObserver.hpp
    CommandSender.hpp
    ChaosInterface.hpp
)
target_include_directories(chaos_communicator 
  PRIVATE
  	${plog_SOURCE_DIR}/include
  PUBLIC
    ${cppzmq_SOURCE_DIR}
    ${mogi_SOURCE_DIR}/edisonLibmogiPackage/libmogi/include
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chaos_communicator cppzmq mogi)
