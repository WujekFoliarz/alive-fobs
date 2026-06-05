cmake_minimum_required(VERSION 3.18)

add_library(BS_thread_pool STATIC)
target_sources(BS_thread_pool PRIVATE thread_pool.cpp)

target_include_directories(BS_thread_pool PUBLIC
"${CMAKE_CURRENT_SOURCE_DIR}/../../thread-pool/include"
)