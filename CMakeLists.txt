cmake_minimum_required(VERSION 3.20)

project(file_server_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# =========================================================
# STATIC BUILD
# =========================================================

# A dependency-k statikus library-k legyenek
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Ne a gépen telepített curlt használja
set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "" FORCE)

# Windows saját SSL implementációját használjuk
set(CPR_FORCE_WINSSL_BACKEND ON CACHE BOOL "" FORCE)

# Ne kelljen libpsl dependency
set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)

# =========================================================
# CPR
# =========================================================

FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 1.14.2
)

FetchContent_MakeAvailable(cpr)

# =========================================================
# DOTENV
# =========================================================

FetchContent_Declare(
        dotenv
        GIT_REPOSITORY https://github.com/laserpants/dotenv-cpp.git
        GIT_TAG master
)

FetchContent_MakeAvailable(dotenv)

# =========================================================
# JSON
# =========================================================

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_MakeAvailable(json)

# =========================================================
# EXECUTABLE
# =========================================================

add_executable(
        file_server_cli
        main.cpp
)

target_link_libraries(
        file_server_cli
        PRIVATE
        cpr::cpr
        nlohmann_json::nlohmann_json
)

# dotenv-cpp header-only
target_include_directories(
        file_server_cli
        PRIVATE
        "${dotenv_SOURCE_DIR}/include/laserpants/dotenv"
)

target_compile_definitions(
        file_server_cli
        PRIVATE
        FILE_SERVER_CLI_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

set_target_properties(file_server_cli PROPERTIES
    OUTPUT_NAME "fscli"
)

if(MINGW)
    target_link_options(
            file_server_cli
            PRIVATE
            -static-libgcc
            -static-libstdc++
    )
endif()