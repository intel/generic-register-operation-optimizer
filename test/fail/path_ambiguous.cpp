#include <groov/path.hpp>
#include <groov/value_path.hpp>

// EXPECT: Attempting to access value with an ambiguous path

auto main() -> int {
    using namespace groov::literals;
    constexpr auto v = "reg"_r("field"_f = 5, "field"_f = 10.0);
    constexpr auto x = v["reg.field"_f];
}
