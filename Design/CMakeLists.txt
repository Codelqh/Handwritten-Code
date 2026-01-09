add_library(Singleton INTERFACE)
target_include_directories(Singleton INTERFACE

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>

    $<INSTALL_INTERFACE:include>

)