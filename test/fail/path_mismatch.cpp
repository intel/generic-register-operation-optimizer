#include <groov/path.hpp>

// EXPECT: Attempting to access value with a mismatched path

auto main() -> int {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field1"_f = 5, "field2"_f = 10.0);
    constexpr auto x = v["reg.foo"_f];
}
