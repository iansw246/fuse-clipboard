cmake_minimum_required(VERSION 3.14)

project(FuseClipboardLib LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui)

add_library(FuseClipboardLib
    fuse.cpp fuse.hpp
    qtClipboardData.cpp
    qtClipboardData.hpp
    qtClipboardDataBase.cpp
    qtClipboardDataBase.hpp
)

# C++ 17
target_compile_features(FuseClipboardLib PUBLIC cxx_std_17)

find_library(FUSE3 fuse3 HINTS "/usr/lib/libfuse3.so")
find_path(FUSE3_INCLUDE_DIR "fuse3/fuse.h" HINTS "/usr/include/fuse3/fuse.h")

target_include_directories(FuseClipboardLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${FUSE3_INCLUDE_DIR}")

message(STATUS "FUSE3_INCLUDE_DIR=${FUSE3_INCLUDE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT FUSE3)
  message(FATAL_ERROR "fuse3 library not found")
endif()

target_link_libraries(FuseClipboardLib PUBLIC Qt${QT_VERSION_MAJOR}::Gui ${FUSE3})

target_compile_options(FuseClipboardLib PRIVATE -Wall -Wextra -Wpedantic)
