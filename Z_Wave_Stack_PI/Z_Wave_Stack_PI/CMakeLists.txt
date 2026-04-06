# CMakeList.txt : CMake project for Z_Wave_Stack_PI, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable(Z_Wave_Stack_PI
    "pch.h"
    "pch.cpp"
    "Initialize.h"
    "Initialize.cpp"
    
    "APICommands.cpp"
    "Controller.cpp"
    "ControllerInfo.cpp"
    "Interface.cpp"
    "SerialPort.cpp"
    "Logging.cpp"
    "Z_Wave_Stack_PI.cpp"
    "ZWaveAPI.cpp"
    "APICommands.h"
    
    "APICommands.h"
    "APIFrame.h"
    "CommandClass.h"
    "Controller.h"
    "ControllerInfo.h"
    "FormatCompat.h"
    "Initialize.h"
    "Interface.h"
    "NodeId_t.h"
    "SerialPort.h"
    "Logging.h"
    "Notify.h"
    "Z_Wave_Stack_PI.h"
    "ZWaveAPI.h" "Nodes.h" "Nodes.cpp" "NodeInfo.h" "NodeInfo.cpp" "NodeInterview.h" "NodeInterview.cpp" "Node.h" "Node.cpp" "NodeCommandClasses.h" "NodeCommandClasses.cpp" "ControllerInfoFormatter.cpp")

if (MSVC)
  string(REPLACE "/W3" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

target_compile_features(Z_Wave_Stack_PI PRIVATE cxx_std_20)
target_compile_options(Z_Wave_Stack_PI PRIVATE
  $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:MSVC>>:/W4 /WX>
  $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:-Wall -Werror>
)
set_target_properties(Z_Wave_Stack_PI PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)

# Enable precompiled headers for faster builds
target_precompile_headers(Z_Wave_Stack_PI PRIVATE "pch.h")

# TODO: Add tests and install targets if needed.
