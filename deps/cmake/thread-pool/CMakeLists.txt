cmake_minimum_required(VERSION 4.2.1)
project(BS_thread_pool_test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/permissive- /Zc:__cplusplus)
endif()

add_library(BS_thread_pool)
target_sources(BS_thread_pool PRIVATE FILE_SET CXX_MODULES BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../thread-pool/modules" FILES ../../thread-pool/modules/BS.thread_pool.cppm)
target_include_directories(BS_thread_pool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../thread-pool/include")