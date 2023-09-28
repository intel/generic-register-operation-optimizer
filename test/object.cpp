#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <groov/object.hpp>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

TEST(object, construction) {
    groov::object r{};
}
