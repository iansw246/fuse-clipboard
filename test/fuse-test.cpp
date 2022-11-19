#include "fuse.hpp"
#include <gtest/gtest.h>

// Demonstrate some basic assertions.
TEST(FuseTest, GetAttr_Root) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

TEST(FuseTest, fullMimeTypeToFileName) {
    const std::string fileName = FuseImplementation::fullMimeTypeToFileName("text/plain");
    EXPECT_EQ(fileName, "file.plain");
}

TEST(FuseTest, filePathToFullMimeType) {
    const std::string fullMimeType = FuseImplementation::filePathToFullMimeType("image/file.png");
    EXPECT_EQ(fullMimeType, "image/png");
}
