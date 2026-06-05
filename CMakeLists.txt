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

find_package(SQLite3 QUIET)

if(NOT SQLite3_FOUND)
  message(STATUS "SQLite3 not found, fetching via FetchContent...")
  include(FetchContent)
  FetchContent_Declare(
    sqlite_amalgamation
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip
  )
  FetchContent_MakeAvailable(sqlite_amalgamation)
  add_library(sqlite3 STATIC ${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c)
  target_include_directories(sqlite3 PUBLIC ${sqlite_amalgamation_SOURCE_DIR})
  add_library(SQLite::SQLite3 ALIAS sqlite3)
endif()

add_subdirectory(deps/cpp-httplib)
add_subdirectory(deps/sqlpp11)
add_subdirectory(deps/json)
add_subdirectory(deps/cmake)

add_executable(${PROJECT_NAME} ${MY_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC httplib::httplib SQLite::SQLite3 sqlpp11 BS_thread_pool nlohmann_json::nlohmann_json)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(EXISTS "${CMAKE_SOURCE_DIR}/res/.env")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/res/.env ${CMAKE_BINARY_DIR}/.env )
else()
    message(".env file is missing in res folder")
endif()