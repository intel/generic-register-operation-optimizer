#include "dummy_bus.hpp"

#include <groov/config.hpp>
#include <groov/path.hpp>
#include <groov/read_spec.hpp>

#include <cstdint>

// EXPECT: Redundant path passed to group

namespace {
using F0 = groov::field<"field0", std::uint8_t, 0, 0>;

std::uint32_t data0{};
using R0 = groov::reg<"reg0", std::uint32_t, &data0, groov::w::replace, F0>;
std::uint32_t data1{};
using R1 = groov::reg<"reg1", std::uint32_t, &data1, groov::w::replace, F0>;

using G = groov::group<"group", dummy_bus, R0, R1>;
} // namespace

auto main() -> int {
    using namespace groov::literals;
    constexpr auto grp = G{};
    constexpr auto x = grp("reg0"_r, "reg0.field0"_f);
}
