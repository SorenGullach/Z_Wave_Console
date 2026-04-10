# CMakeList.txt : CMake project for Z_Wave_Stack_PI, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable(Z_Wave_Stack_PI
    # Precompiled header
    "pch.h"
    "pch.cpp"

    # Entry point
    "Z_Wave_Stack_PI.cpp"
    "Z_Wave_Stack_PI.h"

    # Serial API and transport
    "APICommands.cpp"
    "APICommands.h"
    "APIFrame.h"
    "CommandClass.h"
    "Interface.cpp"
    "Interface.h"
    "Logging.cpp"
    "Logging.h"
    "Notify.h"
    "SerialPort.cpp"
    "SerialPort.h"
    "ZWaveAPI.cpp"
    "ZWaveAPI.h"

    # Controller and initialization
    "Controller.cpp"
    "Controller.h"
    "ControllerInfo.cpp"
    "ControllerInfo.h"
    "ControllerInfoFormatter.cpp"
    "Initialize.cpp"
    "Initialize.h"

    # Node model and interview
    "Node.cpp"
    "Node.h"
    "NodeCommandClasses.cpp"
    "NodeCommandClasses.h"
    "NodeInfo.cpp"
    "NodeInfo.h"
    "NodeInfoFormatter.cpp"
    "NodeInterview.cpp"
    "NodeInterview.h"
    "Nodes.cpp"
    "Nodes.h"

    # Shared utilities
    "FormatCompat.h"
    "Strong_id.h"
)

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
