cmake_minimum_required(VERSION 3.20)

project(file_server_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)
set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Windows alatt a Windows saját SSL backendje
if(WIN32)
    set(CPR_FORCE_WINSSL_BACKEND ON CACHE BOOL "" FORCE)
endif()


if(MSVC)
    set(
        CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        CACHE STRING ""
        FORCE
    )
endif()


FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.14.2
)

FetchContent_MakeAvailable(cpr)


FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_MakeAvailable(json)


add_executable(
    file_server_cli
    main.cpp
)

target_compile_definitions(
    file_server_cli
    PRIVATE
    PROJECT_ROOT="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(
    file_server_cli
    PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
)

set_target_properties(
    file_server_cli
    PROPERTIES
    OUTPUT_NAME "fscli"
)
