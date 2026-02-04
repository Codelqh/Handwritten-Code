
add_library(LRU INTERFACE)

# 设置头文件路径（使其他目标 include 时能找到）
target_include_directories(LRU INTERFACE

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>

    $<INSTALL_INTERFACE:include>

)