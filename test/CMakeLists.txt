include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    fuse-test
    fuse-test.cpp
)

target_link_libraries(
    fuse-test
    GTest::gtest_main
    FuseClipboardLib
)

include(GoogleTest)
gtest_discover_tests(fuse-test)
