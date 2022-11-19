cmake_minimum_required(VERSION 3.14)

add_executable(fuse-clipboard
  main.cpp
)

# Requried for fsanitize address compiler option
target_link_libraries(fuse-clipboard
    FuseClipboardLib
    "$<$<CONFIG:Debug>:-fsanitize=address>"
)
target_compile_options(fuse-clipboard PRIVATE "$<$<CONFIG:Debug>:-fsanitize=address>")

target_compile_options(fuse-clipboard PRIVATE -Wall -Wextra -Wpedantic)

install(TARGETS fuse-clipboard
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
