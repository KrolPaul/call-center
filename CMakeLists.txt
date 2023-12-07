cmake_minimum_required(VERSION 3.2)

set(PROJECT_NAME CallCenter)
project(${PROJECT_NAME})

include_directories(
    "include"
    "external"
    "external/easylogging++"
)
# Define log macro here
add_compile_definitions(
#    ELPP_DISABLE_LOGS
    ELPP_DISABLE_DEBUG_LOGS
#    ELPP_DISABLE_INFO_LOGS
#    ELPP_DISABLE_WARNING_LOGS
#    ELPP_DISABLE_ERROR_LOGS
#    ELPP_DISABLE_FATAL_LOGS
)

add_executable(${PROJECT_NAME}
    external/easylogging++/easylogging++.cc
    external/httplib.h

    src/call-center.cpp
    src/http-server.cpp
    src/cdr.cpp
    src/rand-generator.hpp
    src/main.cpp
)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
add_subdirectory(googletest-release-1.11.0)
add_subdirectory(tests)
