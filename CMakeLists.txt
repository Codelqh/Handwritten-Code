cmake_minimum_required(VERSION 3.14)
project(HandWritten)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 自动导出 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 启用测试（可选）
enable_testing()

# 添加子目录（模块）
add_subdirectory(Pointer)
add_subdirectory(RingBuffer)
add_subdirectory(Timer)
add_subdirectory(Example)   # 示例依赖上面的库