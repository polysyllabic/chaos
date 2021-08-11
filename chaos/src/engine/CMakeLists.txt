target_sources(chaos-engine PRIVATE src/chaosEngine.cpp
			    	    src/chaosUhid.cpp
				    src/commandListener.cpp
				    src/commandSender.cpp
				    src/controller.cpp
				    src/controllerRaw.cpp
				    src/controllerState.cpp
				    src/device.cpp
				    src/interface.cpp
				    src/mouse.cpp
				    src/joystick.cpp
				    src/sequence.cpp
				    include/chaosUhid.hpp
				    include/chaosInterface.hpp
				    include/controllerState.hpp
				    include/controllerRaw.hpp
			     PUBLIC include/chaosEngine.hpp
			     	    include/sequence.hpp
				    include/controller.hpp
				    include/device.hpp
				    include/deviceTypes.hpp
				    )

if(USE_DUALSENSE)
   target_sources(chaos-engine PRIVATE src/controllerStateDualsense.cpp
   				       include/controllerStateDualsense.hpp)
endif()

if(USE_DUALSHOCK)
   target_sources(chaos-engine PRIVATE src/controllerStateDualshock.cpp
       				       include/controllerStateDualshock.hpp)
endif()

