cmake_minimum_required(VERSION 3.21)
project(SpriteStitcher VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Kept on by default (not just for a one-off cleanup pass) so new code
# can't quietly regress. Gated by compiler ID rather than applied
# unconditionally since MSVC uses different flags for the same thing —
# relevant once a Windows build exists.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

enable_testing()

add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(app)
