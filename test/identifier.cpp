#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <groov/identifier.hpp>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

using namespace groov::literals;

TEST(identifier, construction) {
    "field_name"_f;
    "register_name"_r;
    "reg"_r / "field"_f;
    "reg"_r / "field"_f = 10;

    "reg"_r("field1"_f, "field2"_f = 5);

    EXPECT_EQ(10, ("field"_f = 10).value);
    EXPECT_EQ(42, ("hello"_f = 42).value);

    // TODO: equality
    // TODO: "reg.field"_f == "reg"_r / "field"_f
}
