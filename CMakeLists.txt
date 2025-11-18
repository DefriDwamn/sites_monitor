cmake_minimum_required(VERSION 3.31.7)

project(monitor CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(BOOST_INCLUDE_LIBRARIES system asio)
include(FetchContent)
FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.89.0/boost-1.89.0-cmake.7z
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(Boost)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.hpp")

include_directories(include)
add_executable(${PROJECT_NAME} "main.cpp" ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system Boost::asio)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()
