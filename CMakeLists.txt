cmake_minimum_required (VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project (alive-fobs)
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# CROSS PLATFORM LIBRARIES

find_package(SQLite3 REQUIRED)

add_subdirectory(deps/cpp-httplib)
add_subdirectory(deps/sqlpp11)
add_subdirectory(deps/json)
add_subdirectory(deps/cmake)

add_executable(${PROJECT_NAME} ${MY_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC httplib::httplib SQLite3::SQLite3 sqlpp11 json BS_thread_pool)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_SOURCE_DIR}/res/.env
  ${CMAKE_BINARY_DIR}/.env
)