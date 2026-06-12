cmake_minimum_required(VERSION 3.15)
project(tdesktop VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# PAM is optional for compilation (runtime fallback available)
find_package(PAM QUIET)
if(NOT PAM_FOUND)
    message(STATUS "PAM not found - shadow authentication will be used as fallback")
    # Create dummy target for conditional compilation
    add_library(PAM::PAM INTERFACE IMPORTED)
endif()

option(USE_NCURSES "Use ncurses for terminal control" OFF)
option(USE_LIBNOTIFY "Use libnotify for notification" OFF)
option(USE_LIBX11 "Use X11 for mouse reading" OFF)

add_subdirectory(src)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
