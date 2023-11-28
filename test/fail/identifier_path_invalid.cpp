#include <groov/identifier.hpp>

// EXPECT: invalid path

auto main() -> int {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    constexpr auto x = v["reg.foo"_f];
}
