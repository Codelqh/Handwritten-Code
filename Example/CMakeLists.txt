# 编译一个或多个示例
add_executable(shared_ptr_example shared_ptr_example.cpp)
add_executable(unique_ptr_example unique_ptr_example.cpp)
add_executable(timer_example timer_example.cpp)
add_executable(singleton_example singleton_example.cpp)

# 链接所需模块（自动包含头文件路径）
target_link_libraries(shared_ptr_example
    PRIVATE
    Pointer
)

target_link_libraries(unique_ptr_example
    PRIVATE
    Pointer
)

target_link_libraries(timer_example
    PRIVATE
    Timer
)

target_link_libraries(singleton_example
    PRIVATE
    Singleton
)