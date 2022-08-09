cmake_minimum_required(VERSION 3.14)

project(fuse-clipboard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui)

add_executable(fuse-clipboard
  main.cpp
  fuse.hpp
  fuse.cpp
  clipboardInfo.hpp
)

# C++ 17
target_compile_features(fuse-clipboard PUBLIC cxx_std_17)

find_library(FUSE3 fuse3)
target_include_directories(fuse-clipboard PRIVATE "/usr/include/fuse3")

target_link_libraries(fuse-clipboard Qt${QT_VERSION_MAJOR}::Gui ${FUSE3})

install(TARGETS fuse-clipboard
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
