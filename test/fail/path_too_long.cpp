#include <groov/path.hpp>

// EXPECT: too-long path

auto main() -> int {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    constexpr auto x = v["reg.field1.foo"_f];
}
