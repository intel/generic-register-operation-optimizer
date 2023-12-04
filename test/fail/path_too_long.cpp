#include <groov/path.hpp>

// EXPECT: Attempting to access value with a path that is too long

auto main() -> int {
    using namespace groov::literals;
    constexpr auto v = ("reg.field1"_f = 5);
    constexpr auto x = v["reg.field1.foo"_f];
}
