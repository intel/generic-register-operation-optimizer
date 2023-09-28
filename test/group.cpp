#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <groov/group.hpp>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

TEST(group, construction) {
    groov::group r{};
}
