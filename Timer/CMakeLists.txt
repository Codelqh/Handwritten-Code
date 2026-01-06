add_library(Timer INTERFACE)

target_include_directories(Timer INTERFACE

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>

    $<INSTALL_INTERFACE:include>

)